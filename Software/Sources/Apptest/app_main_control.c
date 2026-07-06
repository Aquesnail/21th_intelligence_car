#include "FreeRTOS.h"
#include "task.h"

#include "config.h"
#include "adc_user.h"
#include "debug_arch.h"
#include "motor.h"
#include "motor_register_defs.h"
#include "spi3_user.h"
#include <string.h>
#include "imu_filter.h"
#include "lpf.h"
#include "moving_avg.h"
#include "fuzzy_pid.h"
#include "element.h"
#include "led_indicator.h"
#include "pwm_user.h"
#pragma warning disable = 174

/* ================================================================
 * 控制管道类型定义
 * ================================================================ */

/* 内环 PI 参数 */
typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float error_sum_max;
    float min_sep;
    float max_sep;
} PIParams_t;

/* 内环 PI 状态 */
typedef struct {
    float error_sum;
    float last_error;
} PIState_t;

/* ================================================================
 * 控制参数分组 (按状态机劫持点切分)
 *   AdcErrParams:    劫持点0 之前 — ADC 误差权重 (各元素可屏蔽不同电感)
 *   OuterLoopParams: 劫持点1→劫持点3 — 外环全流程
 *   ControlParams:   完整参数集 = outer + adc + inner, 每个元素配一套
 * ================================================================ */

/* 劫持点0: ADC 误差计算参数 */
typedef struct {
    float   err_A_K;
    float   err_B_K;
    float   err_C_K;
    float   err_D_K;
} AdcErrParams_t;

/* 劫持点1→劫持点3: 外环控制参数 (不含 Kd_diff, 它在 RunOuterLoop 内作为局部变量) */
typedef struct {
    float   outer_slew_max;
    float   outer_lpf_fc;
    float   gyro_lpf_fc;
    float   Kd_diff_high;
    float   Kd_diff_low;
    float   Kd_diff_thresh;
    float   diff_deadband;
    uint8_t d_ma_window;
    float   outer_Ki;
    float   outer_i_limit;
    float   outer_i_sep_hi;
    float   outer_i_sep_lo;
} OuterLoopParams_t;

/* 速度输出后处理参数 (每个元素独立配置) */
typedef struct {
    float max_output;   /* 单轮最大输出限幅 (绝对值)               */
    float min_output;   /* 单轮最小输出限幅 (死区, 低于此值置零)    */
    float max_diff;     /* 最大差速限幅 (自动 ≤ common_speed 防反转) */
    float min_diff;     /* 最小差速限幅 (死区, |diff| 低于此值置零)  */
    float slew_rate;    /* 输出斜率限制 (每 tick 最大变化量)         */
} SpeedOutputParams_t;

/* 完整控制参数集 (一个元素配一套, 上位机在线调参) */
typedef struct {
    OuterLoopParams_t   outer;
    AdcErrParams_t      adc;
    PIParams_t          inner;
    SpeedOutputParams_t speed_out;  /* 速度输出后处理 */
} ControlParams_t;

/* 控制管线运行时状态 (每帧变化) */
typedef struct {
    float   err_slewed;
    float   err_filtered;
    Lpf_t   lpf_outer;
    float   last_err;
    MovingAvg_t d_ma;
    float   filtered_d_term;
    float   i_sum;
    float   i_term;
    Lpf_t   lpf_gyro;
    float   gyro_filtered;
    float   tar_yaw_raw;
    float   target_yaw;
    float   pid_output;
    PIState_t inner;
    float   last_spd_l;   /* 左轮上帧输出 (slew 限制用) */
    float   last_spd_r;   /* 右轮上帧输出 (slew 限制用) */
} ControlState_t;

/* ================================================================
 * 控制管道参数 + 状态 (Phase 3: 集中管理, 替代散落变量)
 * ================================================================ */
/* 元素专用控制参数 (cross 走 g_params_cross, 其余回落 g_params) */
static ControlParams_t g_params;         /* 默认参数 */
static ControlParams_t g_params_cross;   /* 十字专用 (屏蔽竖直电感) */
static ControlState_t  g_state;          /* 运行时 memset 初始化 */
static const ControlParams_t *g_active_params_prev = NULL; /* 上一帧参数集, 用于检测切换 */
static motor_data_t speed_target_l; /* 左轮速度目标 (被 ApplySpeedOutputParams / CtrlState_Seed 引用) */
static motor_data_t speed_target_r; /* 右轮速度目标 */


static void copy_params(ControlParams_t *dest, const ControlParams_t *src)
{
    memcpy(dest, src, sizeof(ControlParams_t));
}

/* 用指定值 seed 整个管线 — 退出冻结/退出开环时调用, 保证连续性 */
static void CtrlState_Seed(ControlState_t *s, const ControlParams_t *p, float seed_val)
{
    (void)p;
    s->err_slewed      = seed_val;
    s->last_err        = seed_val;
    s->i_sum           = 0.0f;
    s->i_term          = 0.0f;
    s->filtered_d_term = 0.0f;
    lpf_reset(&s->lpf_outer, seed_val);
    ma_reset(&s->d_ma, 0.0f);
    s->inner.error_sum  = 0.0f;
    s->inner.last_error = 0.0f;
    s->last_spd_l       = speed_target_l.f;
    s->last_spd_r       = speed_target_r.f;
}


/* ================================================================
 * GetActiveParams — 根据元素返回对应参数集指针
 *   返回值变化时会触发 CtrlState_Seed 平滑过渡
 * ================================================================ */
static const ControlParams_t* GetActiveParams(element_t e)
{
    switch (e) {
    case ELEMENT_CROSS:
        return &g_params_cross;
    default:
        return &g_params;
    }
}

/* ================================================================
 * ApplyParamsIfChanged — 参数集切换时 seed 管线, 防止突变抖动
 * ================================================================ */
static void ApplyParamsIfChanged(const ControlParams_t *p)
{
    if (p != g_active_params_prev) {
        CtrlState_Seed(&g_state, p, g_state.err_filtered);
        g_active_params_prev = p;
    }
}


/* ================================================================
 * 速度目标值宏 (临时占位, 后续调整)
 * ================================================================ */

/* ================================================================
 * TaskNotify 信号位定义
 * ================================================================ */


/* ================================================================
 * 全局变量
 * ================================================================ */

/* ADC DMA 缓冲区 */
static uint8_t xdata adc_dma_buf[ADC2_DMASIZE]; // 运行时 memset 初始化

/* ADC 各通道采样值 */
static float adc_values[ADC2_CH_NUM]; // 运行时 memset 初始化

/* 电机数据 (500Hz stream 推送) */
static float    g_left_speed_actual  = 0.0f;
static float    g_right_speed_actual = 0.0f;
static uint32_t g_mech_state         = 0;

/* 低通滤波器实例 (左右轮速度) */
static Lpf_t g_lpf_left;
static Lpf_t g_lpf_right;

/* 速度目标 & 误差 */
static motor_data_t g_left_speed_target;  // 运行时 memset 初始化
static motor_data_t g_right_speed_target; // 运行时 memset 初始化
static float g_left_speed_error  = 0.0f;
static float g_right_speed_error = 0.0f;

/* PID 参数延迟写入 (回调只置标志, Main_Control 安全点执行 SPI 写入, 避免竞态) */
static volatile uint8_t g_reg_write_pending = 0;
static uint8_t          g_reg_write_addr;
static motor_data_t     g_reg_write_value;

/* 手动模式超时保护 (3s 无指令自动刹车) */
static TickType_t g_last_manual_cmd_tick = 0;
static uint8_t    g_manual_cmd_received  = 0;

/* Stream 模式状态位 (0=已关闭, 1=已开启) */
static volatile uint8_t g_stream_active = 0;

/* 自动模式过渡状态: 0=空闲, 1=风扇已启动等待3s, 2=电机已激活 */
static uint8_t      g_auto_transition = 0;
static TickType_t   g_auto_fan_tick   = 0;



/* 任务句柄 */
TaskHandle_t xMainControlTaskHandle = NULL;

/* ================================================================
 * SPI3 DMA 回调 (预留)
 * ================================================================ */
void SPI3_Callback(void *pvParameters)
{
    (void)pvParameters;
}
/* PID 参数外设描述符 (Motor A / 左电机) */
float xdata g_pid_a_kp = 0.0f;
float xdata g_pid_a_ki = 0.0f;
float xdata g_pid_a_kd = 0.0f;
float xdata g_pid_a_ilim = 3.0f;
float xdata g_pid_b_kp = 0.0f;
float xdata g_pid_b_ki = 0.0f;
float xdata g_pid_b_kd = 0.0f;
float xdata g_pid_b_ilim = 3.0f;
/* ================================================================
 * 电机寄存器 读/写 回调 (供 Debug 系统 PERI_TYPE 使用)
 *   上位机请求 STATIC_REFRESH 时触发读, 写变量时触发写
 * ================================================================ */
static void Motor_Reg_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    float tmp;
    switch(peri_addr){
        case REG_A_PID_SPEED_KP:
            tmp = g_pid_a_kp;
            break;
        case REG_A_PID_SPEED_KI:
            tmp = g_pid_a_ki;
            break;
        case REG_A_PID_SPEED_KD:
            tmp = g_pid_a_kd;
            break;
        case REG_A_PID_SPEED_INTEGRAL_LIMIT:
            tmp = g_pid_a_ilim;
            break;
        case REG_B_PID_SPEED_KP:
            tmp = g_pid_b_kp;
            break;
        case REG_B_PID_SPEED_KI:
            tmp = g_pid_b_ki;
            break;
        case REG_B_PID_SPEED_KD:
            tmp = g_pid_b_kd;
            break;
        case REG_B_PID_SPEED_INTEGRAL_LIMIT:
            tmp = g_pid_b_ilim;
            break;
        default:
						break;
             /* 未识别寄存器地址, 返回0 */
    }
   // tmp = 0.0f;
    memcpy(pData,&tmp,sizeof(float));
    return;
}

static void Motor_Reg_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    (void)len;
    memcpy(&g_reg_write_value.u32, pData, sizeof(uint32_t));
    g_reg_write_addr = (uint8_t)peri_addr;
    switch(g_reg_write_addr){
        case REG_A_PID_SPEED_KP:
            g_pid_a_kp = g_reg_write_value.f;
            break;
        case REG_A_PID_SPEED_KI:
            g_pid_a_ki = g_reg_write_value.f;
            break;
        case REG_A_PID_SPEED_KD:
            g_pid_a_kd = g_reg_write_value.f;
            break;
        case REG_A_PID_SPEED_INTEGRAL_LIMIT:
            g_pid_a_ilim = g_reg_write_value.f;
            break;
        case REG_B_PID_SPEED_KP:
            g_pid_b_kp = g_reg_write_value.f;
            break;
        case REG_B_PID_SPEED_KI:
            g_pid_b_ki = g_reg_write_value.f;
            break;
        case REG_B_PID_SPEED_KD:
            g_pid_b_kd = g_reg_write_value.f;
            break;
        case REG_B_PID_SPEED_INTEGRAL_LIMIT:
            g_pid_b_ilim = g_reg_write_value.f;
            break;
        default:
            break;
    }
    g_reg_write_pending = 1;
    g_last_manual_cmd_tick = xTaskGetTickCount();
    g_manual_cmd_received = 1;
}

/* ================================================================
 * 内环 PI 控制器 (角速度)
 * ================================================================ */
float PI_Step(const PIParams_t *params, PIState_t *state, float setpoint, float actual)
{
    float error;
    float output;
    float d_term;
    error = setpoint - actual;
    state->error_sum += params->Ki * error;
    if (state->error_sum > params->error_sum_max)  state->error_sum = params->error_sum_max;
    if (state->error_sum < -params->error_sum_max) state->error_sum = -params->error_sum_max;
    if (error < params->min_sep && error > -params->min_sep) state->error_sum = 0;
    if (error > params->max_sep || error < -params->max_sep) state->error_sum = 0;
    d_term = params->Kd * (error - state->last_error);
    state->last_error = error;
    output = params->Kp * error + state->error_sum + d_term;
    return output;
}

/*服务于上位机的EC移动平均窗口调整器*/
static void EC_MA_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t val;
    (void)len;
    val = (uint16_t)(*(char *)(peri_addr));
    memcpy(pData, &val, sizeof(uint16_t));
}

static void EC_MA_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t new_val;
    unsigned char count;
    (void)len;
    memcpy(&new_val, pData, sizeof(uint16_t));
    count = (unsigned char)new_val;
    if (count < 1) count = 1;
    if (count > 50) count = 50;
    *(char *)(peri_addr) = (char)count;
}
static float normalized_adc_k[4] = {1.0306345224380493f, 1.0166940689086914f, 0.9944114089012146f, 0.9619343280792236f}; // ADC 归一化系数数组
static uint8_t normalize_index = 0;
static uint8_t adc_start_normalize = 0;
static uint8_t change_inductor = 1;
static float roll=0.0f, pitch=0.0f, yaw=0.0f;
static float adc_avg_sum = 0.0f;
static float adc_avg = 0.0f;
static uint16_t adc_avg_count = 0;
static uint8_t display_adc_original = 0;
static float adc_normalized[4]; // 运行时 memset 初始化
static void NormalizeADC(uint32_t peri_addr,void *pData,uint8_t len){
    uint8_t* ptr = (uint8_t *)pData;
    (void)peri_addr;
    (void)len;
    if(*ptr<4){
        normalize_index = *ptr;
        adc_avg_sum = 0.0f;
        adc_avg_count = 0;
    }
    adc_start_normalize = 1;
    return;
}

uint8_t ADC_NormalizeCommand(const char* cmd) {
    if (strcmp(cmd, "norm0") == 0) {
        normalize_index = 0;
        NormalizeADC(0, &normalize_index, sizeof(normalize_index));
        Debug_Log("ADC_NORM_0");
        return 1;
    } else if (strcmp(cmd, "norm1") == 0) {
        normalize_index = 1;
        NormalizeADC(0, &normalize_index, sizeof(normalize_index));
        Debug_Log("ADC_NORM_1");
        return 1;
    } else if (strcmp(cmd, "norm2") == 0) {
        normalize_index = 2;
        NormalizeADC(0, &normalize_index, sizeof(normalize_index));
        Debug_Log("ADC_NORM_2");
        return 1;
    } else if (strcmp(cmd, "norm3") == 0) {
        normalize_index = 3;
        NormalizeADC(0, &normalize_index, sizeof(normalize_index));
        Debug_Log("ADC_NORM_3");
        return 1;
    }
    return 0; // 未识别
}
static uint16_t fan_D = 350;
static void Fan_D_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t new_val;
    (void)len;
    (void)peri_addr;
    memcpy(&new_val, pData, sizeof(uint16_t));
    fan_D = new_val;
    //PWMA_Set(fan_D);
}

static void Fan_D_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t val;
    (void)len;
    (void)peri_addr;
    val = fan_D;
    memcpy(pData, &val, sizeof(uint16_t));
}

/* ================================================================
 * 控制管道 API
 * ================================================================ */

/* 全量复位 — 刹车/滑行/切手动时调用 */
static void CtrlState_Reset(ControlState_t *s, const ControlParams_t *p)
{
    (void)p;
    s->err_slewed      = 0.0f;
    s->err_filtered    = 0.0f;
    s->last_err        = 0.0f;
    s->filtered_d_term = 0.0f;
    s->i_sum           = 0.0f;
    s->i_term          = 0.0f;
    s->tar_yaw_raw     = 0.0f;
    s->target_yaw      = 0.0f;
    s->gyro_filtered   = 0.0f;
    lpf_reset(&s->lpf_outer, 0.0f);
    lpf_reset(&s->lpf_gyro,  0.0f);
    ma_reset(&s->d_ma, 0.0f);
    s->inner.error_sum  = 0.0f;
    s->inner.last_error = 0.0f;
    s->last_spd_l       = 0.0f;
    s->last_spd_r       = 0.0f;
}



/* 冻结/保持模式 — 进入开环或冻结时调用, 清零积分+微分 */
static void CtrlState_Hold(ControlState_t *s)
{
    s->i_sum           = 0.0f;
    s->i_term          = 0.0f;
    s->last_err        = s->err_filtered;
    s->filtered_d_term = 0.0f;
    ma_reset(&s->d_ma, 0.0f);
}
/* 外环 LPF 截止频率读写回调 (上位机在线调参) */
static void OuterLpf_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    float val;
    (void)len;
    (void)peri_addr;
    val = g_params.outer.outer_lpf_fc;
    memcpy(pData, &val, sizeof(float));
}

static void OuterLpf_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    float new_fc;
    float alpha;
    (void)len;
    (void)peri_addr;
    memcpy(&new_fc, pData, sizeof(float));
    if (new_fc < 1.0f)  new_fc = 1.0f;
    if (new_fc > 400.0f) new_fc = 400.0f;
    g_params.outer.outer_lpf_fc = new_fc;
    alpha = 1.0f - (float)exp(-6.2831853f * new_fc * 0.002f);
    g_state.lpf_outer.alpha = alpha;
    g_state.lpf_outer.freq = new_fc;
    /* 不重置 output, 保持滤波连续性 */
}
/* 微分移动平均窗口读写回调 (上位机在线调参) */
static void D_MA_Win_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t val;
    (void)len;
    (void)peri_addr;
    val = (uint16_t)g_params.outer.d_ma_window;
    memcpy(pData, &val, sizeof(uint16_t));
}

static void D_MA_Win_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t new_val;
    (void)len;
    (void)peri_addr;
    memcpy(&new_val, pData, sizeof(uint16_t));
    if (new_val > MA_MAX_WINDOW) new_val = MA_MAX_WINDOW;
    g_params.outer.d_ma_window = (uint8_t)new_val;
    ma_set_window(&g_state.d_ma, g_params.outer.d_ma_window);
}
static DebugPeriDesc_t g_pid_a_kp_desc   = {REG_A_PID_SPEED_KP,             Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};
static DebugPeriDesc_t g_pid_a_ki_desc   = {REG_A_PID_SPEED_KI,             Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};
static DebugPeriDesc_t g_pid_a_kd_desc   = {REG_A_PID_SPEED_KD,             Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};
static DebugPeriDesc_t g_pid_a_ilim_desc = {REG_A_PID_SPEED_INTEGRAL_LIMIT, Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};

/* PID 参数外设描述符 (Motor B / 右电机) */
static DebugPeriDesc_t g_pid_b_kp_desc   = {REG_B_PID_SPEED_KP,             Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};
static DebugPeriDesc_t g_pid_b_ki_desc   = {REG_B_PID_SPEED_KI,             Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};
static DebugPeriDesc_t g_pid_b_kd_desc   = {REG_B_PID_SPEED_KD,             Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};
static DebugPeriDesc_t g_pid_b_ilim_desc = {REG_B_PID_SPEED_INTEGRAL_LIMIT, Motor_Reg_Read_Cb, Motor_Reg_Write_Cb};
static DebugPeriDesc_t g_right_speed_desc = {REG_A_SPEED_TARGET, Motor_Reg_Read_Cb,Motor_Reg_Write_Cb}; // 只写当前右轮速度
static DebugPeriDesc_t g_left_speed_desc  = {REG_B_SPEED_TARGET,Motor_Reg_Read_Cb, Motor_Reg_Write_Cb}; // 只写当前左轮速度
static DebugPeriDesc_t g_ec_ma_count_desc = {0, EC_MA_Read_Cb, EC_MA_Write_Cb}; /* EC移动平均窗口大小 */
static DebugPeriDesc_t g_fan_d = {0, Fan_D_Read_Cb, Fan_D_Write_Cb}; /* 风扇占空比寄存器 */
static DebugPeriDesc_t g_outer_lpf_fc_desc = {0, OuterLpf_Read_Cb, OuterLpf_Write_Cb}; /* 外环LPF截止频率 */
static DebugPeriDesc_t g_d_ma_win_desc     = {0, D_MA_Win_Read_Cb, D_MA_Win_Write_Cb}; /* 微分MA窗口 */
extern IMU_Filter_t *IMU_Filter_Data;

motor_data_t motor_spi_dat_handle(uint8_t* buffer)
{
    motor_data_t dat;
    int8_t i;
    dat.u32 = 0;
    for (i = 4-1; i >= 0; i--) {
        dat.u32 |= (uint32_t)buffer[4-1-i] << (i * 8);
    }
    return dat;
}
/* ================================================================
 * Main_Control_Task
 *   2ms 固定周期
 *   1. 读取 ADC 并重启
 *   2. 写入左右电机速度目标
 *   3. 计算速度误差供上位机监测
 * ================================================================ */
uint8_t spi_buffer[12];
float left_speed_target = 0.0f;
float right_speed_target = 0.0f;
uint8_t manual_control_flag = 1;


uint16_t adc_dead_zone = 5;
float adc_err_zone = 0.5f;
float Err_Angle=0.0f;

/* 驾驶方向命令 (标志位方式, Main_Control 安全点执行) */
#define DRIVE_STOP     0
#define DRIVE_FORWARD  1
#define DRIVE_BACKWARD 2
#define DRIVE_LEFT     3
#define DRIVE_RIGHT    4
float  drive_speed  = 10.0f;

static volatile uint8_t g_drive_direction = DRIVE_STOP;

uint8_t Motor_HandleCommand(const char* cmd)
{
    if (strcmp(cmd, "forward") == 0) {
        g_drive_direction = DRIVE_FORWARD;
        Debug_Log("MOTOR_FWD");
    } else if (strcmp(cmd, "backward") == 0) {
        g_drive_direction = DRIVE_BACKWARD;
        Debug_Log("MOTOR_BCK");
    } else if (strcmp(cmd, "left") == 0) {
        g_drive_direction = DRIVE_LEFT;
        Debug_Log("MOTOR_LFT");
    } else if (strcmp(cmd, "right") == 0) {
        g_drive_direction = DRIVE_RIGHT;
        Debug_Log("MOTOR_RGT");
    } else if (strcmp(cmd, "stop") == 0) {
        g_drive_direction = DRIVE_STOP;
        Debug_Log("MOTOR_STP");
    } else {
        return 0;
    }
    /* 刷新手动模式超时计时 */
    g_last_manual_cmd_tick = xTaskGetTickCount();
    g_manual_cmd_received = 1;
    return 1;
}

static FuzzyPID pid_struct; // 运行时 memset 初始化
static float pid_output=0.0f;
static float common_speed=50.0f;
/* 外环带宽限制: 斜率限制 + 低通滤波 — 参数g_params + 状态g_state */

/* PI 已移入 g_params.inner + g_state.inner */
static float speed_target_r_nagtive=0.0f;
static float err_max = 120.0f;
static element_t current_element = ELEMENT_STRAIGHT;
static element_t prev_element    = ELEMENT_STRAIGHT;  /* 上一次元素, 用于检测切换 */
static ControlStrategy_t prev_ctrl = CTRL_NORMAL;     /* 上一次控制策略, 用于检测切换 */

static ElementCtx_t g_ectx;  /* 元素状态机上下文 */

static float tmp_adc13;
static float tmp_adcsum;

#define spd_limit 150.0f

/* ================================================================
 * 缓启动 (Soft-Start): 限制加速度, 防止启动电流冲击
 *   g_ss_magnitude: 当前缓启动输出速度幅值
 *   g_ss_step:      加速步进/周期 (2ms 周期)
 *   SS_STEP_DOWN:   减速步进 (快速响应刹车)
 * ================================================================ */
static float g_ss_magnitude = 0.0f;
static float g_ss_step      = 2.0f;
#define SS_STEP_DOWN 10.0f



/* ================================================================
 * ApplySpeedOutputParams — 速度输出后处理
 *   输入: common_speed (查表速度), diff_speed (内环PI输出)
 *   处理: 差速死区 → 差速限幅 → 速度合成 → 单轮限幅 → 斜率限制
 *   输出: 写入全局 speed_target_l.f / speed_target_r.f
 * ================================================================ */
static void ApplySpeedOutputParams(const SpeedOutputParams_t *p, ControlState_t *s,
                                    float common_speed, float *diff_speed)
{
    float raw_l, raw_r;
    float tgt_l, tgt_r;
    float diff_l, diff_r;
    float eff_max_diff;

    /* ---- 1. 差速死区 ---- */
    if (*diff_speed < p->min_diff && *diff_speed > -p->min_diff) {
        *diff_speed = 0.0f;
    }

    /* ---- 2. 差速限幅 ---- */
    eff_max_diff = p->max_diff;
    // if (eff_max_diff > common_speed) {
    //     eff_max_diff = common_speed;
    // }
    if (*diff_speed >  eff_max_diff) *diff_speed =  eff_max_diff;
    if (*diff_speed < -eff_max_diff) *diff_speed = -eff_max_diff;

    /* ---- 3. 速度合成 ---- */
    raw_l = common_speed - *diff_speed;
    raw_r = -(common_speed + *diff_speed); //后面都是对称的判断，所以这里取负号没有问题

    /* ---- 4. 单轮输出限幅 ---- */
    if (raw_l >  p->max_output) raw_l =  p->max_output;
    if (raw_l < -p->max_output) raw_l = -p->max_output;
    if (raw_r >  p->max_output) raw_r =  p->max_output;
    if (raw_r < -p->max_output) raw_r = -p->max_output;

    /* 单轮死区 */
    if (raw_l < p->min_output && raw_l > -p->min_output) raw_l = 0.0f;
    if (raw_r < p->min_output && raw_r > -p->min_output) raw_r = 0.0f;

    /* ---- 5. 斜率限制 ---- */
    tgt_l = raw_l;
    tgt_r = raw_r;
    diff_l = tgt_l - s->last_spd_l;
    diff_r = tgt_r - s->last_spd_r;

    if (diff_l >  p->slew_rate)      tgt_l = s->last_spd_l + p->slew_rate;
    if (diff_l < -p->slew_rate)      tgt_l = s->last_spd_l - p->slew_rate;
    if (diff_r >  p->slew_rate)      tgt_r = s->last_spd_r + p->slew_rate;
    if (diff_r < -p->slew_rate)      tgt_r = s->last_spd_r - p->slew_rate;

    /* 存储状态 */
    s->last_spd_l = tgt_l;
    s->last_spd_r = tgt_r;

    /* 输出到全局速度目标 */
    speed_target_l.f = tgt_l;
    speed_target_r.f = tgt_r;
}


/* 统一速度写入包装: 传入逻辑速度, 内部用 WR_SIGN 适配下位机方向 */
/* ================================================================
 * 速度方向适配宏 (下位机方向约定变更时只需改这 4 个宏)
 *   WR_SIGN: 写入下位机的最终符号
 *   RD_SIGN: 上位机读取/显示的符号
 * ================================================================ */
#define SPD_WR_SIGN_R  (-1)   /* 右电机 (REG_A) 写入符号 */
#define SPD_WR_SIGN_L  (-1)   /* 左电机 (REG_B) 写入符号 */
#define SPD_RD_SIGN_R  (-1)   /* 右电机 读取/显示符号 */
#define SPD_RD_SIGN_L  (1)    /* 左电机 读取/显示符号 */
static void Motor_Write_Speeds(float spd_r, float spd_l)
{
    motor_data_t dr, dl;
    dr.f = spd_r * SPD_WR_SIGN_R;
    dl.f = spd_l * SPD_WR_SIGN_L;
    motor_write_speed_targets(dr, dl);
}

/* 缓启动步进: 加速缓 (step_up), 减速快 (step_down) */
static float SoftStart_Ramp(float cur, float tgt, float step_up, float step_down)
{
    float diff;
    float step;
    diff = tgt - cur;
    step = (diff > 0.0f) ? step_up : step_down;
    if (diff > step) {
        return cur + step;
    } else if (diff < -step) {
        return cur - step;
    } else {
        return tgt;
    }
}

void System_Reset(void){   
    /* 复位所有状态机/控制管线/滤波器 */
    CtrlState_Reset(&g_state, &g_params);
    Element_ResetOfftrackTimer(NULL);
    Element_ResetTrackIndex(NULL);
    g_ectx.current = ELEMENT_STRAIGHT;
    g_ectx.exit_lockout_cnt = 0;
    g_ectx.offtrack_active = 0;
    g_ectx.total_ticks_start = xTaskGetTickCount();
    manual_control_flag = 1;
    PWMA_Set(0);
    LED_RequestBreathing();              /* 切手动 → 呼吸灯 */
    g_auto_transition = 0;
    g_ss_magnitude = 0.0f;  /* 重置缓启动 */
}


void Change_Maunal(){
	if(manual_control_flag){
        /* 切换到自动模式: 启动风扇, 2s 后再激活电机 */
        manual_control_flag = 0;
        PWMA_Set(fan_D);
        LED_RequestSteady(255, 255, 255);   /* 切自动 → 稳态白光 */
        g_auto_transition = 1;
        g_auto_fan_tick = xTaskGetTickCount();
        g_ss_magnitude = 0.0f;  /* 重置缓启动 */
        CtrlState_Reset(&g_state, &g_params);  /* 全量复位控制管线 */
        current_element = ELEMENT_STRAIGHT;
        prev_ctrl = CTRL_NORMAL;                   /* 对齐自动模式初始控制策略 */
        Element_ResetOfftrackTimer(NULL);
        Element_ResetTrackIndex(NULL);
        g_ectx.current = ELEMENT_STRAIGHT;
        g_ectx.exit_lockout_cnt = 0;
        g_ectx.offtrack_active = 0;
        /* total_ticks_start 由电机激活时设定, 不在此处提前设置 */
    }else{
        /* 切换到手动模式: 关闭风扇, 刹车 */
        manual_control_flag = 1;
        PWMA_Set(0);
        LED_RequestBreathing();              /* 切手动 → 呼吸灯 */
        g_auto_transition = 0;
        g_ss_magnitude = 0.0f;  /* 重置缓启动 */
    }
}

extern void IMU_Get_Angle(float* roll, float* pitch, float* yaw);
extern float IMU_Get_Yaw_Unwrapped(void);
extern float IMU_Get_Gyro_Z(void);
static float sqrt_negative(float value) {

    if (value < 0.0f) {
        return -sqrt(-value);
    } else {
        return sqrt(value);
    }
}
static float abs_f(float t){
    if(t>0.0f){
        return t;
    }else{
        return -t;
    }
}
/* err_angle_filtered 已移入 g_state.err_filtered */

/* ================================================================
 * OnCtrlTransition — 控制策略切换时的状态清理
 *   集中管理所有 PID/滤波器状态复位, 替代原来散落在
 *   SAFETY_STOP case / Change_Maunal / 自动过渡 中的重复代码
 * ================================================================ */
static void OnCtrlTransition(ControlStrategy_t from, ControlStrategy_t to)
{
    if (from == to) return;

    /* 进入开环: 清零外环积分和微分历史 */
    if (to == CTRL_OL_YAW) {
        CtrlState_Hold(&g_state);
        return;
    }

    /* 开环恢复到正常 PID: 用当前误差 seed 管线 */
    if (from == CTRL_OL_YAW && to == CTRL_NORMAL) {
        CtrlState_Seed(&g_state, &g_params, g_state.err_filtered);
        return;
    }

    /* 进入冻结: 清零 I/D 历史, 冻结在 Err_Angle 层面处理 */
    if (to == CTRL_ERR_FREEZE) {
        CtrlState_Hold(&g_state);
        return;
    }

    /* 退出冻结: seed 管线到当前滤波值 */
    if (from == CTRL_ERR_FREEZE && to == CTRL_NORMAL) {
        CtrlState_Seed(&g_state, &g_params, g_state.err_filtered);
        return;
    }

    /* 进入刹车/滑行: 全量复位 */
    if (to == CTRL_BRAKE ) {
        CtrlState_Reset(&g_state, &g_params);
        return;
    }
}

/* ================================================================
 * ControlDispatch — 统一电机派发
 *   替代原来分散在两处的电机写入逻辑
 * ================================================================ */
static void ControlDispatch(ControlStrategy_t ctrl, float spd_r, float spd_l)
{
    switch (ctrl) {
    case CTRL_NORMAL:
    case CTRL_OL_YAW:
    case CTRL_ERR_FREEZE:
    case CTRL_RA_YAW:
        Motor_Write_Speeds(spd_r, spd_l);
        break;
    case CTRL_BRAKE:
        Motor_Write_Speeds(0.0f, 0.0f);
        PWMA_Set(0);   /* 关负压风扇 */
        break;
    case CTRL_FREEWHEEL:
    default:
        Motor_Write_Speeds(0.0f, 0.0f);
        PWMA_Set(0);
        break;
    }
}

/* ================================================================
 * CalcAdcError — ADC 误差计算 (劫持点0 → 劫持点1 之间)
 *   各元素通过切换 AdcErrParams 屏蔽不同电感通道
 * ================================================================ */
static float CalcAdcError(const AdcErrParams_t *p,
                          float Err_1, float Err_2,
                          float Err_3, float Err_4)
{
    float numerator;
    float denominator;

    numerator   = p->err_A_K * Err_1 + p->err_B_K * Err_2;
    denominator = p->err_C_K * Err_3 + p->err_D_K * Err_4;

    if (denominator < 1.0e-6f && denominator > -1.0e-6f) {
        return 0.0f;
    }
    return numerator / denominator * 100.0f;
}

/* ================================================================
 * RunOuterLoop — 外环全流程 (劫持点1 → 劫持点3)
 *   输入: 劫持后 Err_Angle + 原始陀螺角速度
 *   输出: tar_yaw_raw (待劫持的内环目标角速度)
 *   内含: slew限制+LPF+模糊PID+积分分离+陀螺滤波+不完全微分+自适应Kd
 * ================================================================ */
static float RunOuterLoop(const OuterLoopParams_t *p, ControlState_t *s,
                          FuzzyPID *pid, float err_angle, float gyro_raw)
{
    float Kd_diff;
    float slew_diff;
    float err_diff;
    float raw_d_term;
    /* 1. 斜率限制 → 2. 低通滤波 */
    slew_diff = err_angle - s->err_slewed;
    if (slew_diff > p->outer_slew_max) {
        s->err_slewed += p->outer_slew_max;
    } else if (slew_diff < -p->outer_slew_max) {
        s->err_slewed -= p->outer_slew_max;
    } else {
        s->err_slewed = err_angle;
    }
    s->err_filtered = lpf_update(&s->lpf_outer, s->err_slewed);

    /* 3. 模糊 PID */
    s->pid_output = FuzzyPID_Step(pid, s->err_filtered);

    /* 4. 外环积分 (积分分离) */
    if (abs_f(s->err_filtered) > p->outer_i_sep_hi) {
        s->i_sum = 0.0f;
    } else if (abs_f(s->err_filtered) < p->outer_i_sep_lo) {
        s->i_sum = 0.0f;
    } else {
        s->i_sum += p->outer_Ki * (-s->err_filtered);
        if (s->i_sum > p->outer_i_limit)  s->i_sum = p->outer_i_limit;
        if (s->i_sum < -p->outer_i_limit) s->i_sum = -p->outer_i_limit;
    }
    s->i_term = s->i_sum;

    /* 5. 陀螺仪低通滤波 */
    s->gyro_filtered = lpf_update(&s->lpf_gyro, gyro_raw);

    /* 6. 不完全微分 (MA + 死区) */
    err_diff = s->err_filtered - s->last_err;
    if (err_diff > -p->diff_deadband && err_diff < p->diff_deadband) {
        err_diff = 0.0f;
    }
    raw_d_term = err_diff / 0.002f;
    s->filtered_d_term = ma_update(&s->d_ma, raw_d_term);
    s->last_err = s->err_filtered;

    /* 7. 自适应微分系数选择 (Kd_diff 原为 params 字段, 实为运行时局部变量) */
    if (abs_f(s->err_filtered) > p->Kd_diff_thresh) {
        Kd_diff = p->Kd_diff_high;
    } else {
        Kd_diff = p->Kd_diff_low;
    }

    /* 8. 外环合成: tar_yaw = P输出 + I项 - Kd * 滤波微分 */
    return s->pid_output
         + s->i_term
         - Kd_diff * s->filtered_d_term;
}

/* ================================================================
 * RunInnerLoop — 内环 PI (劫持点3 之后)
 *   输入: 劫持后 target_yaw + 滤波后实际角速度
 *   输出: 左右轮差速
 * ================================================================ */
static float RunInnerLoop(const PIParams_t *p, PIState_t *s,
                          float target_yaw, float actual_yaw)
{
    return PI_Step(p, s, target_yaw, actual_yaw);
}

/* 速度变量已迁移至 element.c, 通过 Element_GetProp 查表获取 */
void Main_Control_Task(void *pvParameters)
{
    uint8_t i;
    uint8_t led_r, led_g, led_b;   /* LED 颜色临时变量 */
    motor_data_t tmp;
    motor_data_t speed_a, speed_b;
    uint32_t ulNotifiedValue;
    uint16_t raw;  /* TODO: 临时, ADC 尖峰过滤用 */
    float Err_1;
    float Err_2;
    float Err_3;
    float Err_4;
    float differential_speed;
    float actual_yaw_rate_raw;  /* 陀螺仪原始角速度 (滤波前) */
    float gx_body, gy_body, gz_body, gz_abs;  /* 重力向量倾角计算 */
    float g_horiz, tilt_total, inv_horiz;     /* 总倾角+分解 (大倾角稳定) */
    const ElementProp_t *prop;              /* 当前元素属性 (每 tick 查一次) */
    ControlStrategy_t    ctrl;              /* 当前有效控制策略 */

    (void)pvParameters;

    Element_Init(&g_ectx);  /* 初始化元素状态机上下文 */

//    xLastWakeTime = xTaskGetTickCount();
    vTaskDelay(pdMS_TO_TICKS(100)); /* 启动后延时 100ms 等待系统稳定 */
      /* 开启电机 stream 模式 */
    MOTOR_CS_RESET();
    motor_stream_start(0x01, 0x02);
    MOTOR_CS_SET();
    while(IMU_Filter_Data->is_calibrated==0){
        vTaskDelay(pdMS_TO_TICKS(10));
        MOTOR_CS_RESET();
        tmp.u32 = 0x01;
        motor_write_reg(0x01, tmp);
        MOTOR_CS_SET();
    }
    


    g_stream_active = 1;
    P2INTE |= PIN_0;
    for (;;) {
        ulNotifiedValue = 0;
        xTaskNotifyWait(0, SIGNAL_MOTOR_DATA_READY, &ulNotifiedValue, pdMS_TO_TICKS(2));

        /* --- 电机数据处理 (仅在有信号时) --- */
        if (ulNotifiedValue & SIGNAL_MOTOR_DATA_READY) {
            if (g_stream_active) {
                
                MOTOR_CS_RESET();
                for (i = 0; i < 12; i++) {
                    spi_buffer[i] = MOTOR_SPI_TxRx(0xFFu);
                }
                MOTOR_CS_SET();
                tmp = motor_spi_dat_handle(spi_buffer);
                g_right_speed_actual = SPD_RD_SIGN_R * lpf_update(&g_lpf_right, tmp.f);
                tmp = motor_spi_dat_handle(spi_buffer + 4);
                g_left_speed_actual = SPD_RD_SIGN_L * lpf_update(&g_lpf_left, tmp.f);
                tmp = motor_spi_dat_handle(spi_buffer + 8);
                g_mech_state = tmp.u32;

                if(adc_start_normalize){
                    adc_avg_sum += adc_values[normalize_index];
                    adc_avg_count++;
                    if(adc_avg_count >= 500){
                        adc_avg = adc_avg_sum / adc_avg_count;
                        adc_avg_sum = 0;
                        adc_avg_count = 0;
                        if(adc_avg > 800){
                            normalized_adc_k[normalize_index] = 1000.0f / (float)adc_avg; // 简单线性归一化
                        }else{
                            normalized_adc_k[normalize_index] = 1.0f; // 不进行归一化
                        }
                        adc_start_normalize = 0;
                    }
                }
                if(display_adc_original){
                    adc_normalized[0] = adc_values[0];
                    adc_normalized[1] = adc_values[1];
                    adc_normalized[2] = adc_values[2];
                    adc_normalized[3] = adc_values[3];
                }else{
                    adc_normalized[0] = (adc_values[0] < adc_dead_zone) ? 0.0f : adc_values[0] * normalized_adc_k[0];
                    adc_normalized[1] = (adc_values[1] < adc_dead_zone) ? 0.0f : adc_values[1] * normalized_adc_k[1];
                    adc_normalized[2] = (adc_values[2] < adc_dead_zone) ? 0.0f : adc_values[2] * normalized_adc_k[2];
                    adc_normalized[3] = (adc_values[3] < adc_dead_zone) ? 0.0f : adc_values[3] * normalized_adc_k[3];
                }
                tmp_adc13 = adc_normalized[1]+adc_normalized[3];
                tmp_adcsum = adc_normalized[0]+adc_normalized[1]+adc_normalized[2]+adc_normalized[3];
                if(change_inductor == 1){
                    Err_1 = (float)((adc_normalized[0]) - (adc_normalized[2]));//0,2对应横向电感
                    Err_2 = (float)((adc_normalized[1]) - (adc_normalized[3]));//1,3对应竖直电感
                    Err_3 = (float)(adc_normalized[0] + adc_normalized[2]);
                    Err_4 = abs_f((float)(adc_normalized[1] - adc_normalized[3]));
                }else{
                    Err_1 = (float)((adc_normalized[1]) - (adc_normalized[3]));
                    Err_2 = (float)((adc_normalized[0]) - (adc_normalized[2]));
                    Err_3 = (float)(adc_normalized[1] + adc_normalized[3]);
                    Err_4 = abs_f((float)(adc_normalized[0] - adc_normalized[2]));
                }
                
                if(Err_1 < adc_err_zone && Err_1 > -adc_err_zone) Err_1 = 0.0f;
                if(Err_2 < adc_err_zone && Err_2 > -adc_err_zone) Err_2 = 0.0f;
                if(Err_3 < adc_err_zone && Err_3 > -adc_err_zone) Err_3 = 0.0f;
                if(Err_4 < adc_err_zone && Err_4 > -adc_err_zone) Err_4 = 0.0f;
                /* ===== Block 0: ADC \xce�\xb2�\xbc�\xcb� (\xb0�\xd4�\xcb�\xd1�\xb2�) ===== */
                {
                    const ControlParams_t *p = GetActiveParams(current_element);
                    Err_Angle = CalcAdcError(&p->adc, Err_1, Err_2, Err_3, Err_4);
                }
                /* 填充元素状态机的 err 观测值 (先于 Element_Update + 冻结) */
                g_ectx.err_raw      = Err_Angle;
                g_ectx.err_filtered = g_state.err_filtered; //这里必须更新原始值，不能劫持
                 /* ===== 元素状态机更新 (先于控制决策, 保证本帧状态最新) ===== */
                {
                    unsigned char j;
                    for (j = 0; j < 4; j++) {
                        g_ectx.adc[j] = adc_normalized[j];
                    }
                    g_ectx.ax  = IMU_Filter_Data->sensor.dat.ax;
                    g_ectx.ay  = IMU_Filter_Data->sensor.dat.ay;
                    g_ectx.az  = IMU_Filter_Data->sensor.dat.az;
                    g_ectx.yaw = yaw;

                    IMU_Get_Angle(&roll, &pitch, &yaw);
                    yaw = IMU_Get_Yaw_Unwrapped();

                    /* ---- 四元数重力向量倾角 (总倾角+按比例分解, 大倾角稳定) ---- */
                    IMU_GetGravityVector(&gx_body, &gy_body, &gz_body);

                    /* 水平分量幅值: g_horiz²+gz²=1 (四元数归一化保证) */
                    g_horiz = (float)sqrt(gx_body * gx_body + gy_body * gy_body);
                    gz_abs  = (gz_body > 0.0f) ? gz_body : -gz_body;

                    /* 总倾角 = atan2(水平, 垂直) — 大倾角时 g_horiz→1, 无小分母 */
                    tilt_total = (float)atan2(g_horiz, gz_abs) * 57.29578f;

                    /* 按比例分解为前后/左右分量, 保留符号区分方向 */
                    if (g_horiz > 0.001f) {
                        inv_horiz = 1.0f / g_horiz;
                        g_ectx.tilt_fb = tilt_total * gx_body * inv_horiz;
                        g_ectx.tilt_lr = tilt_total * gy_body * inv_horiz;
                    } else {
                        g_ectx.tilt_fb = 0.0f;
                        g_ectx.tilt_lr = 0.0f;
                    }
                    current_element = Element_Update(&g_ectx);
                }
                /* 直角转弯冻结: CTRL_ERR_FREEZE 策略下用冻结值替代真实 Err_Angle */
/************    状态机插入 ****************** */

                Err_Angle = SelectTargetErr(ctrl, prop, &g_ectx);

/************    状态机插入 ****************** */


                /* ===== Block 1: params lookup + ApplyParamsIfChanged ===== */
                {
                    const ControlParams_t *p = GetActiveParams(current_element);
                    ApplyParamsIfChanged(p);
                }
/* 每 tick 一次查表 (替代原多处分散的 Element_GetProp 调用) */
                prop = Element_GetProp(current_element);
                ctrl = Element_GetEffectiveCtrl(current_element);

                /* 元素切换 → 更新 LED (仅自动模式, 手动模式保持呼吸灯) */
                if (!manual_control_flag && current_element != prev_element) {
                    LED_GetElementColor(current_element, &led_r, &led_g, &led_b);
                    if (current_element == ELEMENT_SAFETY_STOP) {
                        LED_RequestBreathing();
                    } else {
                        LED_RequestSteady(led_r, led_g, led_b);
                    }
                    prev_element = current_element;
                }

/************    状态机插入 ****************** */

                /* 控制策略切换 → 清理 PID 状态 (替代原散落的复位代码) */
                if (ctrl != prev_ctrl) {
                    OnCtrlTransition(prev_ctrl, ctrl);
                    prev_ctrl = ctrl;
                }
/************    状态机插入 ****************** */



                /* 速度查表 (基于最新状态机输出) */
                if (prop != NULL && prop->speed_ptr != NULL) {
                    common_speed = *(prop->speed_ptr);
                }

                /* ===== Block 2: �⻷ȫ���� �� tar_yaw_raw ===== */
                {
                    const ControlParams_t *p = GetActiveParams(current_element);
                    actual_yaw_rate_raw = IMU_Get_Gyro_Z();
                    g_state.tar_yaw_raw = RunOuterLoop(&p->outer, &g_state, &pid_struct,
                                                        Err_Angle, actual_yaw_rate_raw);
                }
//角速度环路劫持
                /* 统一目标角速度选择 (替代原 OL_YAW 劫持) */
/***************    状态机插入 ****************** */

                g_ectx.yawrate = g_state.tar_yaw_raw;
                g_state.target_yaw = SelectTargetYaw(ctrl, prop,&g_ectx);


/***************   状态机插入 ****************** */


                /* 内环 PI + 速度合成 (唯一一次, 基于最新状态机输出) */
                {
                    const ControlParams_t *p = GetActiveParams(current_element);
                    differential_speed = RunInnerLoop(&p->inner, &g_state.inner,
                                                       g_state.target_yaw, g_state.gyro_filtered);
                    ApplySpeedOutputParams(&p->speed_out, &g_state,
                                            common_speed, &differential_speed);
                }
                speed_target_r_nagtive = -speed_target_r.f;

                /* 处理 Debug 系统延迟的 PID 参数写入 / 手动模式超时保护 / 驾驶指令 */
                if(manual_control_flag){
                    /* 3s 无指令自动刹车 */
                    if (g_manual_cmd_received &&
                        (xTaskGetTickCount() - g_last_manual_cmd_tick) >= pdMS_TO_TICKS(3000)) {
                        tmp.f = 0.0f;
                        Motor_Write_Speeds(0.0f, 0.0f);
                        g_reg_write_pending = 0;
                        g_drive_direction = DRIVE_STOP;
                        g_last_manual_cmd_tick = xTaskGetTickCount();
                        g_ss_magnitude = 0.0f;  /* 重置缓启动 */
                    }

                    /* 缓启动: ramp 速度幅值 */
                    if (g_drive_direction != DRIVE_STOP) {
                        g_ss_magnitude = SoftStart_Ramp(g_ss_magnitude, drive_speed, g_ss_step, SS_STEP_DOWN);
                    } else {
                        g_ss_magnitude = SoftStart_Ramp(g_ss_magnitude, 0.0f, g_ss_step, SS_STEP_DOWN);
                    }

                    /* 执行驾驶指令 (标志位 -> 电机写入) */
                    if (g_drive_direction != DRIVE_STOP) {
                        if (g_drive_direction == DRIVE_FORWARD) {
                            speed_a.f = -g_ss_magnitude;
                            speed_b.f = g_ss_magnitude;
                        } else if (g_drive_direction == DRIVE_BACKWARD) {
                            speed_a.f = g_ss_magnitude;
                            speed_b.f = -g_ss_magnitude;
                        } else if (g_drive_direction == DRIVE_LEFT) {
                            speed_a.f = -g_ss_magnitude;
                            speed_b.f = -g_ss_magnitude;
                        } else if (g_drive_direction == DRIVE_RIGHT) {
                            speed_a.f = g_ss_magnitude;
                            speed_b.f = g_ss_magnitude;
                        }
                        Motor_Write_Speeds(speed_a.f, speed_b.f);
                    }else{
                            tmp.f = 0.0f;
                            Motor_Write_Speeds(0.0f, 0.0f);
                    }
                    if (g_reg_write_pending) {
                        if(g_reg_write_addr == REG_A_SPEED_TARGET){
                            g_reg_write_value.f *= SPD_WR_SIGN_R;
                        }
                        MOTOR_CS_RESET();
                        motor_write_reg(g_reg_write_addr, g_reg_write_value);
                        MOTOR_CS_SET();
                        g_reg_write_pending = 0;
                    }
                    
                }else{
                    /* --- 自动模式: 处理延迟 PID 参数写入 --- */
                    if (g_reg_write_pending) {
                        MOTOR_CS_RESET();
                        motor_write_reg(g_reg_write_addr, g_reg_write_value);
                        MOTOR_CS_SET();
                        g_reg_write_pending = 0;
                    }

                    /* 风扇启动后等待 2s 再激活电机 */
                    if (g_auto_transition == 1) {
                        if ((xTaskGetTickCount() - g_auto_fan_tick) >= pdMS_TO_TICKS(2000)) {
                            g_auto_transition = 2;
                            lpf_reset(&g_state.lpf_gyro, 0.0f);
                            g_state.inner.error_sum = 0.0f;
                            g_state.inner.last_error = 0.0f;
                            g_state.i_sum = 0.0f;
                            g_ectx.total_ticks_start = xTaskGetTickCount();
                        }
                    }

                    /* 统一电机派发 (状态机 + PID 已在上面统一计算) */
                    if (g_auto_transition == 2) {
                        ControlDispatch(ctrl, speed_target_r.f, speed_target_l.f);
                    }

                }
                //taskEXIT_CRITICAL();
            }

            if (ADC2_DMA_IsDone()) {
                    
                ADC2_DMA_ClearFlag();
                
                for (i = 0; i < ADC2_CH_NUM; i++) {
                    raw = ADC2_GetAvgValue(i);
                    adc_values[i] = (float)raw; //最右横向为1，最左横向为3，左中为2，右中为0
                }
                
                ADC2_DMA_Start();
               
            }
        }
    }
}

/* ================================================================
 * P2.0 中断处理 (500Hz 电机数据就绪)
 *   通知 Main_Control_Task
 * ================================================================ */
void Motor_Data_Ready_INT2_Handle(BaseType_t *xHigherPriorityTaskWoken)
{
    xTaskNotifyFromISR(xMainControlTaskHandle, SIGNAL_MOTOR_DATA_READY, eSetBits, xHigherPriorityTaskWoken);
}

/* ================================================================
 * 初始化: 注册调试变量 + 创建两个任务
 * ================================================================ */
void Main_Control_Task_Init(void)
{
    motor_data_t tmp;

    /* 运行时初始化大数组/结构体，避免 Flash 镜像占用 */
    memset(adc_dma_buf,          0, sizeof(adc_dma_buf));
    memset(adc_values,           0, sizeof(adc_values));
    memset(&g_left_speed_target,  0, sizeof(g_left_speed_target));
    memset(&g_right_speed_target, 0, sizeof(g_right_speed_target));
    memset(adc_normalized,       0, sizeof(adc_normalized));
    memset(&pid_struct,          0, sizeof(pid_struct));
    memset(&g_params,            0, sizeof(g_params));
    memset(&g_params_cross,      0, sizeof(g_params_cross));

   
    memcpy(&g_params_cross, &g_params, sizeof(ControlParams_t));

    memset(&g_state,             0, sizeof(g_state));
    memset(&speed_target_l,      0, sizeof(speed_target_l));
    memset(&speed_target_r,      0, sizeof(speed_target_r));

    // g_ec_ma_count_desc.peri_addr = (uint32_t)&(pid_struct.ec_count); /* EC移动平均已移除 */
    /* ================================================================
     * 1. 实时观测 (HIGH_FREQ) — 只看不改
     * ================================================================ */
    /* ADC */
    Debug_Register("adc0",   DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[0]);
    Debug_Register("adc1",   DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[1]);
    Debug_Register("adc2",   DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[2]);
    Debug_Register("adc3",   DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[3]);
    Debug_Register("adc1+3", DBG_FLOAT | HIGH_FREQ_TYPE, &tmp_adc13);
    Debug_Register("adcsum", DBG_FLOAT | HIGH_FREQ_TYPE, &tmp_adcsum);
    /* 姿态 */
    Debug_Register("yaw",    DBG_FLOAT | HIGH_FREQ_TYPE, &yaw);
    Debug_Register("ErrFil", DBG_FLOAT | HIGH_FREQ_TYPE, &g_state.err_filtered);
    Debug_Register("TiltFB", DBG_FLOAT | HIGH_FREQ_TYPE, &g_ectx.tilt_fb);
    Debug_Register("TiltLR", DBG_FLOAT | HIGH_FREQ_TYPE, &g_ectx.tilt_lr);
    /* 电机 */
    Debug_Register("L_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &(speed_target_l.f));
    Debug_Register("R_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &(speed_target_r_nagtive));
    Debug_Register("L_Act", DBG_FLOAT | HIGH_FREQ_TYPE, &g_left_speed_actual);
    Debug_Register("R_Act", DBG_FLOAT | HIGH_FREQ_TYPE, &g_right_speed_actual);

    /* ================================================================
     * 2. ADC 误差权重 & 归一化 — 默认值 & Debug 注册
     * ================================================================ */
    /* 默认参数 (g_params.adc) */
    g_params.adc.err_A_K = 2.0f;
    g_params.adc.err_B_K = 6.0f;
    g_params.adc.err_C_K = 3.0f;
    g_params.adc.err_D_K = 3.0f;
    Debug_Register("Err_K_A", DBG_FLOAT | STATIC_TYPE, &g_params.adc.err_A_K);
    Debug_Register("Err_K_B", DBG_FLOAT | STATIC_TYPE, &g_params.adc.err_B_K);
    Debug_Register("Err_K_C", DBG_FLOAT | STATIC_TYPE, &g_params.adc.err_C_K);
    Debug_Register("Err_K_D", DBG_FLOAT | STATIC_TYPE, &g_params.adc.err_D_K);

    /* 十字专用 (g_params_cross.adc, Init 末尾 copy 后个别清零) */
    Debug_Register("Cr_ErrKA", DBG_FLOAT | STATIC_TYPE, &g_params_cross.adc.err_A_K);
    Debug_Register("Cr_ErrKB", DBG_FLOAT | STATIC_TYPE, &g_params_cross.adc.err_B_K);
    Debug_Register("Cr_ErrKC", DBG_FLOAT | STATIC_TYPE, &g_params_cross.adc.err_C_K);
    Debug_Register("Cr_ErrKD", DBG_FLOAT | STATIC_TYPE, &g_params_cross.adc.err_D_K);

    /* 死区 & 通道选择 */
    Debug_Register("CH_Induct",   DBG_UINT8  | STATIC_TYPE, &change_inductor);
    Debug_Register("ADC_DeadZone",DBG_UINT16 | STATIC_TYPE, &adc_dead_zone);
    Debug_Register("ADC_ErrZone", DBG_FLOAT  | STATIC_TYPE, &adc_err_zone);

    /* 归一化系数 */
    Debug_Register("NormK", DBG_FLOAT | STATIC_TYPE, &normalized_adc_k[0]);
    Debug_Register("NormK", DBG_FLOAT | STATIC_TYPE, &normalized_adc_k[1]);
    Debug_Register("NormK", DBG_FLOAT | STATIC_TYPE, &normalized_adc_k[2]);
    Debug_Register("NormK", DBG_FLOAT | STATIC_TYPE, &normalized_adc_k[3]);

    /* ================================================================
     * 3. 外环控制参数 (OuterLoop) — 默认值 & Debug 注册
     * ================================================================ */
    g_params.outer.outer_slew_max  = 100.0f;
    g_params.outer.outer_lpf_fc    = 100.0f;
    g_params.outer.gyro_lpf_fc     = 100.0f;
    g_params.outer.Kd_diff_thresh  = 100.0f;
    g_params.outer.diff_deadband   = 0.5f;
    g_params.outer.d_ma_window     = 80;
    g_params.outer.outer_Ki        = 0.23f;
    g_params.outer.outer_i_limit   = 3.6f;
    g_params.outer.outer_i_sep_hi  = 10000.0f;
    g_params.outer.outer_i_sep_lo  = 30.0f;
    Debug_Register("OutSlew",   DBG_FLOAT  | STATIC_TYPE, &g_params.outer.outer_slew_max);
    Debug_Register("OutLPFFc",  DBG_FLOAT  | STATIC_TYPE | PERI_TYPE, &g_outer_lpf_fc_desc);
    Debug_Register("KdDifHi",   DBG_FLOAT  | STATIC_TYPE, &g_params.outer.Kd_diff_high);
    Debug_Register("KdDifLo",   DBG_FLOAT  | STATIC_TYPE, &g_params.outer.Kd_diff_low);
    Debug_Register("KdDifTh",   DBG_FLOAT  | STATIC_TYPE, &g_params.outer.Kd_diff_thresh);
    Debug_Register("DiffDead",  DBG_FLOAT  | STATIC_TYPE, &g_params.outer.diff_deadband);
    Debug_Register("D_MA_Win",  DBG_UINT16 | STATIC_TYPE | PERI_TYPE, &g_d_ma_win_desc);
    Debug_Register("Out_Ki",    DBG_FLOAT  | STATIC_TYPE, &g_params.outer.outer_Ki);
    Debug_Register("OutILim",   DBG_FLOAT  | STATIC_TYPE, &g_params.outer.outer_i_limit);
    Debug_Register("OutIHi",    DBG_FLOAT  | STATIC_TYPE, &g_params.outer.outer_i_sep_hi);
    Debug_Register("OutILo",    DBG_FLOAT  | STATIC_TYPE, &g_params.outer.outer_i_sep_lo);
    Debug_Register("GyroLPFFc", DBG_FLOAT  | STATIC_TYPE, &g_params.outer.gyro_lpf_fc);

    /* ================================================================
     * 4. 外环观测 (HIGH_FREQ) — 只看不改
     * ================================================================ */
    Debug_Register("TgtYaw",  DBG_FLOAT | HIGH_FREQ_TYPE, &g_state.pid_output);
    Debug_Register("TarYawR", DBG_FLOAT | HIGH_FREQ_TYPE, &g_state.tar_yaw_raw);
    Debug_Register("ActYaw",  DBG_FLOAT | HIGH_FREQ_TYPE, &g_state.gyro_filtered);
    Debug_Register("OutITrm", DBG_FLOAT | HIGH_FREQ_TYPE, &g_state.i_term);

    /* ================================================================
     * 5. 内环 PI — 默认值 & Debug 注册
     * ================================================================ */
    g_params.inner.Kp = 5.5f;
    g_params.inner.Ki = 0.0f;
    g_params.inner.Kd = 0.10000000149011612f;
    g_params.inner.error_sum_max = 0.0f;
    g_params.inner.max_sep = 100000.0f;
    g_params.inner.min_sep = 0.0f;
    g_state.inner.error_sum  = 0.0f;
    g_state.inner.last_error = 0.0f;
    Debug_Register("Yaw_Kp",   DBG_FLOAT | STATIC_TYPE, &g_params.inner.Kp);
    Debug_Register("Yaw_Ki",   DBG_FLOAT | STATIC_TYPE, &g_params.inner.Ki);
    Debug_Register("Yaw_Kd",   DBG_FLOAT | STATIC_TYPE, &g_params.inner.Kd);
    Debug_Register("YawLimit", DBG_FLOAT | STATIC_TYPE, &g_params.inner.error_sum_max);
    Debug_Register("YawHiLi",  DBG_FLOAT | STATIC_TYPE, &g_params.inner.max_sep);
    Debug_Register("YawLoLi",  DBG_FLOAT | STATIC_TYPE, &g_params.inner.min_sep);

    /* ================================================================
     * 5b. 速度输出后处理 — 默认值 & Debug 注册
     * ================================================================ */
    g_params.speed_out.max_output = 100.0f;   /* 远大于正常速度, 仅防极端 */
    g_params.speed_out.min_output = 0.0f;     /* 默认无死区 */
    g_params.speed_out.max_diff   = 50.0f;    /* = common_speed, 防止反转 */
    g_params.speed_out.min_diff   = 0.0f;     /* 默认无死区 */
    g_params.speed_out.slew_rate  = 20.0f;    /* 每 tick ±20, 500Hz 下平滑 */
    g_state.last_spd_l = 0.0f;
    g_state.last_spd_r = 0.0f;
    Debug_Register("SpdMax",  DBG_FLOAT | STATIC_TYPE, &g_params.speed_out.max_output);
    Debug_Register("SpdMin",  DBG_FLOAT | STATIC_TYPE, &g_params.speed_out.min_output);
    Debug_Register("DiffMax", DBG_FLOAT | STATIC_TYPE, &g_params.speed_out.max_diff);
    Debug_Register("DiffMin", DBG_FLOAT | STATIC_TYPE, &g_params.speed_out.min_diff);
    Debug_Register("SpdSlew", DBG_FLOAT | STATIC_TYPE, &g_params.speed_out.slew_rate);

    /* ================================================================
     * 6. 模糊 PID — 默认值 & Debug 注册
     * ================================================================ */
    pid_struct.e_max    = 150.0f;
    pid_struct.Kp0      = 0.06300000101327896f;
    pid_struct.setpoint = 0.0f;
    FuzzyPID_Init(&pid_struct, pid_struct.Kp0, pid_struct.e_max);
    /* 覆盖模糊 Kp 单点值 (FKp NB~PB) */
    pid_struct.kp_singletons[FUZZY_NB] = 2.0999999046325684f;
    pid_struct.kp_singletons[FUZZY_NM] = 1.2000000476837158f;
    pid_struct.kp_singletons[FUZZY_NS] = 1.100000023841858f;
    pid_struct.kp_singletons[FUZZY_ZO] = 1.0f;
    pid_struct.kp_singletons[FUZZY_PS] = 1.100000023841858f;
    pid_struct.kp_singletons[FUZZY_PM] = 1.2000000476837158f;
    pid_struct.kp_singletons[FUZZY_PB] = 2.0999999046325684f;
    Debug_Register("FKp0",   DBG_FLOAT | STATIC_TYPE, &pid_struct.Kp0);
    Debug_Register("Fe_max", DBG_FLOAT | STATIC_TYPE, &pid_struct.e_max);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NB]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NM]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NS]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_ZO]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PS]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PM]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PB]);

    /* ================================================================
     * 7. 电机外设 (PERI_TYPE) — 通过 SPI 读写电机寄存器
     * ================================================================ */
    Debug_Register("A_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_ilim_desc);
    Debug_Register("B_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_ilim_desc);

    /* ================================================================
     * 8. 系统 / 杂项 — Debug 注册
     * ================================================================ */
    Debug_Register("ComSpd",    DBG_FLOAT  | STATIC_TYPE, &common_speed);
    Debug_Register("Drive_Spd", DBG_FLOAT  | STATIC_TYPE, &drive_speed);
    Debug_Register("Fan_D",     DBG_UINT16 | STATIC_TYPE | PERI_TYPE, &g_fan_d);
    Debug_Register("SS_Step",   DBG_FLOAT  | STATIC_TYPE, &g_ss_step);
    Debug_Register("Manual",    DBG_UINT8  | STATIC_TYPE, &manual_control_flag);
    Debug_Register("stat",      DBG_UINT16,                &g_ectx.current);

    /* ================================================================
     * 9. 滤波器 & 移动平均初始化 (依赖外环默认参数)
     * ================================================================ */
    lpf_init_fc(&g_lpf_left,              400.0f, 0.002f);
    lpf_init_fc(&g_lpf_right,             400.0f, 0.002f);
    lpf_init_fc(&g_state.lpf_outer, g_params.outer.outer_lpf_fc, 0.002f);
    lpf_init_fc(&g_state.lpf_gyro,  g_params.outer.gyro_lpf_fc,  0.002f);
    ma_init(&g_state.d_ma, g_params.outer.d_ma_window);

    /* ================================================================
     * 10. 十字参数集 & 收尾
     * ================================================================ */
    g_stream_active = 0;
    copy_params(&g_params_cross, &g_params);
    g_params_cross.adc.err_B_K = 0.0f;
    g_params_cross.adc.err_D_K = 0.0f;

    /* 初始化 SPI3, ADC */
    SPI3_Init(SPI3_Callback);
    ADC2_DMA_Init(adc_dma_buf);
    ADC2_DMA_Start();
    //vTaskDelay(pdMS_TO_TICKS(10));
    
    /* 创建 Main_Control_Task (低优先级, 2ms 周期) */
     xTaskCreate(Main_Control_Task, "main_control", 1224, NULL, tskIDLE_PRIORITY + 3, &xMainControlTaskHandle);
}
