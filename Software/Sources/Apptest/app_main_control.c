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
#include "pwm_user.h"
#pragma warning disable = 174
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
static uint8_t xdata adc_dma_buf[ADC2_DMASIZE] = {0};

/* ADC 各通道采样值 */
static float adc_values[ADC2_CH_NUM] = {0};

/* 电机数据 (500Hz stream 推送) */
static float    g_left_speed_actual  = 0.0f;
static float    g_right_speed_actual = 0.0f;
static uint32_t g_mech_state         = 0;

/* 低通滤波器实例 (左右轮速度) */
static Lpf_t g_lpf_left;
static Lpf_t g_lpf_right;

/* 速度目标 & 误差 */
static motor_data_t g_left_speed_target  = {0};
static motor_data_t g_right_speed_target = {0};
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
float xdata g_pid_a_ilim = 2.5f;
float xdata g_pid_b_kp = 0.0f;
float xdata g_pid_b_ki = 0.0f;
float xdata g_pid_b_kd = 0.0f;
float xdata g_pid_b_ilim = 2.5f;
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
typedef struct {
    float Kp;
    float Ki;
    float error_sum;
    float error_sum_max;
    float min_sep;
    float max_sep;
} PI_Controller;

float PI_Step(PI_Controller *pi, float setpoint, float actual)
{
    float error;
    float output;
    error = setpoint - actual;
    pi->error_sum += pi->Ki*error;
    if (pi->error_sum > pi->error_sum_max)  pi->error_sum = pi->error_sum_max;
    if (pi->error_sum < -pi->error_sum_max) pi->error_sum = -pi->error_sum_max;
    if(error < pi->min_sep && error > -pi->min_sep)pi->error_sum = 0;
    if(error > pi->max_sep || error < -pi->max_sep)pi->error_sum = 0;
    output = pi->Kp * error + pi->error_sum;
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
static float normalized_adc_k[4] = {0.9581499695777893f, 1.0557365417480469f, 1.0300761461257935f, 0.9722529053688049f}; // ADC 归一化系数数组
static uint8_t normalize_index = 0;
static uint8_t adc_start_normalize = 0;
static uint8_t change_inductor = 1;
static float roll=0.0f, pitch=0.0f, yaw=0.0f;
static float adc_avg_sum = 0.0f;
static float adc_avg = 0.0f;
static uint16_t adc_avg_count = 0;
static uint8_t display_adc_original = 0;
static float adc_normalized[4] = {0.0f};
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
static uint16_t fan_D = 300;
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
static float g_outer_slew_max = 5.0f;       /* Err_Angle 每周期(2ms)最大变化量 */
static float g_err_angle_slewed = 0.0f;     /* Err_Angle 斜率限制状态 */
static float g_pid_output_filtered = 0.0f;  /* 最终目标角速度 (送入内环) */
static Lpf_t g_lpf_outer;                    /* 外环低通滤波器 */
static float g_outer_lpf_fc = 100.0f;        /* 外环LPF截止频率(Hz), 上位机可调 */
/* 外环方案1+2+3: 陀螺阻尼 + 不完全微分(MA) + 微分死区 */
static float   g_Kd_gyro        = 0.0f;      /* 陀螺阻尼系数 (0=关闭) */
static float   g_Kd_gyro_high   = 0.0f;      /* 陀螺阻尼系数 (大误差时使用) */
static float   g_Kd_gyro_low    = 0.0f;      /* 陀螺阻尼系数 (小误差时使用) */
static float   g_Kd_gyro_thresh = 10.0f;     /* 误差阈值, 超过此值切换为高 Kd */
static float   g_Kd_diff        = 0.0f;      /* 不完全微分系数 (0=关闭) */
static float   g_diff_deadband  = 0.0f;      /* 微分死区阈值 (0=关闭) */
static MovingAvg_t g_d_ma;                   /* 微分项移动平均滤波器 */
static uint8_t g_d_ma_window    = 0;         /* MA窗口 0~100 (0=旁路) */
static float   g_last_err_angle = 0.0f;      /* 上周期滤波后 Err_Angle (微分用) */
static float   g_filtered_d_term = 0.0f;     /* 滤波后微分项 (调试观测) */
static float   g_tar_yaw_raw    = 0.0f;      /* 组合后原始目标角速度 (调试观测) */
/* 外环 LPF 截止频率读写回调 (上位机在线调参) */
static void OuterLpf_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    float val;
    (void)len;
    (void)peri_addr;
    val = g_outer_lpf_fc;
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
    g_outer_lpf_fc = new_fc;
    alpha = 1.0f - (float)exp(-6.2831853f * new_fc * 0.002f);
    g_lpf_outer.alpha = alpha;
    g_lpf_outer.freq = new_fc;
    /* 不重置 output, 保持滤波连续性 */
}
/* 微分移动平均窗口读写回调 (上位机在线调参) */
static void D_MA_Win_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t val;
    (void)len;
    (void)peri_addr;
    val = (uint16_t)g_d_ma_window;
    memcpy(pData, &val, sizeof(uint16_t));
}

static void D_MA_Win_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t new_val;
    (void)len;
    (void)peri_addr;
    memcpy(&new_val, pData, sizeof(uint16_t));
    if (new_val > MA_MAX_WINDOW) new_val = MA_MAX_WINDOW;
    g_d_ma_window = (uint8_t)new_val;
    ma_set_window(&g_d_ma, g_d_ma_window);
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

float Err_A_K=25.0f;  //所有adc 标定值为2200左右
float Err_B_K=120.0f;
float Err_C_K=3.0f;
float Err_D_K=4.0f;
uint16_t adc_dead_zone = 5;
float adc_err_zone = 0.5f;
float Err_Angle=0.0f;

/* 驾驶方向命令 (标志位方式, Main_Control 安全点执行) */
#define DRIVE_STOP     0
#define DRIVE_FORWARD  1
#define DRIVE_BACKWARD 2
#define DRIVE_LEFT     3
#define DRIVE_RIGHT    4
float  drive_speed  = 150.0f;

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

static FuzzyPID pid_struct={0.0f};
static float pid_output=0.0f;
static float common_speed=30.0f;
/* 外环带宽限制: 斜率限制 + 低通滤波 */

static PI_Controller inner_yaw_rate_pi = {0};
static float actual_yaw_rate_debug = 0.0f;
static float pid_output_k = 0.10f;
static motor_data_t speed_target_l={0};
static motor_data_t speed_target_r={0};
static float speed_target_r_nagtive=0.0f;
static float numerator, denominator;
static float err_max = 120.0f;
static element_t current_element = ELEMENT_STRAIGHT;

/* ================================================================
 * 开环过元素 (Open-Loop Element Passing)
 *   由元素状态机驱动: ENTRANCE → PASS → IN_RING → STRAIGHT
 *   PASS 状态下覆盖目标角速度, 内环 PI 继续工作
 *   参数定义在 element.c, 左右环共用一套 (左环取负值)
 * ================================================================ */
extern float g_ol_ring_angle;
extern float g_ol_ring_yawrate;

#define spd_limit 150.0f

/* ================================================================
 * 缓启动 (Soft-Start): 限制加速度, 防止启动电流冲击
 *   g_ss_magnitude: 当前缓启动输出速度幅值
 *   g_ss_step:      加速步进/周期 (2ms 周期)
 *   SS_STEP_DOWN:   减速步进 (快速响应刹车)
 * ================================================================ */
static float g_ss_magnitude = 0.0f;
static float g_ss_step      = 5.0f;
#define SS_STEP_DOWN 10.0f
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

void Change_Maunal(){
	if(manual_control_flag){
        /* 切换到自动模式: 启动风扇, 3s 后再激活电机 */
        manual_control_flag = 0;
        PWMA_Set(fan_D);
        g_auto_transition = 1;
        g_auto_fan_tick = xTaskGetTickCount();
        g_ss_magnitude = 0.0f;  /* 重置缓启动 */
        g_err_angle_slewed = 0.0f;     /* 重置Err_Angle带宽限制 */
        lpf_reset(&g_lpf_outer, 0.0f);
        g_pid_output_filtered = 0.0f;   /* 重置目标角速度 */
        g_last_err_angle = 0.0f;       /* 重置微分状态 */
        ma_reset(&g_d_ma, 0.0f);
    }else{
        /* 切换到手动模式: 关闭风扇, 刹车 */
        manual_control_flag = 1;
        PWMA_Set(0);
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
static float err_angle_filtered = 0.0f;  /* 外环: 滤波后 Err_Angle */
static float cruising_speed = 45.0f;
static float ring_speed  = 45.0f;
static float wall_speed = 30.0f;
static float barrel_speed = 45.0f;
void Main_Control_Task(void *pvParameters)
{
    uint8_t i;
    motor_data_t tmp;
    uint32_t ulNotifiedValue;
    uint16_t raw;  /* TODO: 临时, ADC 尖峰过滤用 */
    float Err_1;
    float Err_2;
    float Err_3;
    float Err_4;
    float differential_speed;
    float actual_yaw_rate;
    float err_diff;        /* 外环微分用 */
    float raw_d_term;      /* 外环微分用 */
    float slew_diff;       /* 外环斜率限制用 */

    (void)pvParameters;
    


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
                //taskENTER_CRITICAL();
                // tmp.f = left_speed_target;
                // g_left_speed_target = tmp;
                // tmp.f = -right_speed_target;
                // g_right_speed_target = tmp;
                IMU_Get_Angle(&roll, &pitch, &yaw);
                yaw = IMU_Get_Yaw_Unwrapped();
                MOTOR_CS_RESET();
                for (i = 0; i < 12; i++) {
                    spi_buffer[i] = MOTOR_SPI_TxRx(0xFFu);
                }
                MOTOR_CS_SET();
                tmp = motor_spi_dat_handle(spi_buffer);
                g_right_speed_actual = -lpf_update(&g_lpf_right, tmp.f);
                tmp = motor_spi_dat_handle(spi_buffer + 4);
                g_left_speed_actual = lpf_update(&g_lpf_left, tmp.f);
                tmp = motor_spi_dat_handle(spi_buffer + 8);
                g_mech_state = tmp.u32;

                if(adc_start_normalize){
                    adc_avg_sum += adc_values[normalize_index];
                    adc_avg_count++;
                    if(adc_avg_count >= 500){
                        adc_avg = adc_avg_sum / adc_avg_count;
                        adc_avg_sum = 0;
                        adc_avg_count = 0;
                        if(adc_avg > 1000){
                            normalized_adc_k[normalize_index] = 2000.0f / (float)adc_avg; // 简单线性归一化
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

                numerator = (Err_A_K*(Err_1)+(Err_B_K*(Err_2)));
                denominator = (Err_C_K*(Err_3))+(Err_D_K*(Err_4));
                if (denominator < 1.0e-6f && denominator > -1.0e-6f) {
                    Err_Angle = 0.0f;
                } else {
                    Err_Angle = numerator / denominator * 100.0f;
                }
                /* 外环带宽限制作用于电感原始数据 Err_Angle: 1.斜率限制 → 2.低通滤波 */
                slew_diff = Err_Angle - g_err_angle_slewed;
                if (slew_diff > g_outer_slew_max) {
                    g_err_angle_slewed += g_outer_slew_max;
                } else if (slew_diff < -g_outer_slew_max) {
                    g_err_angle_slewed -= g_outer_slew_max;
                } else {
                    g_err_angle_slewed = Err_Angle;
                }
                err_angle_filtered = lpf_update(&g_lpf_outer, g_err_angle_slewed);
                pid_output = FuzzyPID_Step(&pid_struct, err_angle_filtered);
                /* 提前读取陀螺仪, 供方案1陀螺阻尼使用 */
                actual_yaw_rate = IMU_Get_Gyro_Z();
                actual_yaw_rate_debug = actual_yaw_rate;
                /* 方案二+三: 不完全微分(MA)+死区, 对滤波后 Err_Angle 取微分 */
                err_diff = err_angle_filtered - g_last_err_angle;
                if (err_diff > -g_diff_deadband && err_diff < g_diff_deadband) {
                    err_diff = 0.0f;  /* 方案三: 微分死区 */
                }
                raw_d_term = err_diff / 0.002f;
                g_filtered_d_term = ma_update(&g_d_ma, raw_d_term);
                g_last_err_angle = err_angle_filtered;  /* 保存滤波后 Err_Angle, 供下一周期微分 */
                /* 根据滤波后误差大小选择陀螺阻尼系数 */
                if (abs_f(err_angle_filtered) > g_Kd_gyro_thresh) {
                    g_Kd_gyro = g_Kd_gyro_high;
                } else {
                    g_Kd_gyro = g_Kd_gyro_low;
                }
                /* 方案一+二+三 组合:
                 *   tar_yaw = P输出 - 陀螺阻尼*实际角速度 - Kd*滤波后微分 */
                g_tar_yaw_raw = pid_output
                              - g_Kd_gyro * actual_yaw_rate
                              - g_Kd_diff * g_filtered_d_term;
                /* g_pid_output_filtered = 最终目标角速度, 送入内环 */
                g_pid_output_filtered = g_tar_yaw_raw;

                /* --- 开环过元素: PASS 状态下覆盖角速度目标 --- */
                if (current_element == ELEMENT_RING_LEFT_PASS) {
                    g_pid_output_filtered = -g_ol_ring_yawrate;
                } else if (current_element == ELEMENT_RING_RIGHT_PASS) {
                    g_pid_output_filtered = g_ol_ring_yawrate;
                }

                differential_speed = PI_Step(&inner_yaw_rate_pi, g_pid_output_filtered, actual_yaw_rate);
                speed_target_l.f = common_speed - differential_speed;
                speed_target_r.f = -(common_speed + differential_speed);
                speed_target_r_nagtive = -speed_target_r.f;
                //最右横向为1，最左横向为3，左中为2，右中为0
                // MOTOR_CS_RESET();
                // motor_write_reg(REG_A_SPEED_TARGET, g_right_speed_target);
                // motor_write_reg(REG_B_SPEED_TARGET, g_left_speed_target);
                // MOTOR_CS_SET();
								
                /* 处理 Debug 系统延迟的 PID 参数写入 / 手动模式超时保护 / 驾驶指令 */
                if(manual_control_flag){
                    /* 3s 无指令自动刹车 */
                    if (g_manual_cmd_received &&
                        (xTaskGetTickCount() - g_last_manual_cmd_tick) >= pdMS_TO_TICKS(3000)) {
                        tmp.f = 0.0f;
                        MOTOR_CS_RESET();
                        motor_write_reg(REG_A_SPEED_TARGET, tmp);
                        motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        MOTOR_CS_SET();
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
                        MOTOR_CS_RESET();
                        if (g_drive_direction == DRIVE_FORWARD) {
                            tmp.f = drive_speed;
                            tmp.f = -g_ss_magnitude;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                           // tmp.f = drive_speed;
                            tmp.f = g_ss_magnitude;
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        } else if (g_drive_direction == DRIVE_BACKWARD) {
                            tmp.f = g_ss_magnitude;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            tmp.f = -g_ss_magnitude;
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        } else if (g_drive_direction == DRIVE_LEFT) {
                            tmp.f = -g_ss_magnitude;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            tmp.f = -g_ss_magnitude;
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        } else if (g_drive_direction == DRIVE_RIGHT) {
                            tmp.f = g_ss_magnitude;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            tmp.f = g_ss_magnitude;
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        }
                        MOTOR_CS_SET();
                    }else{
                            tmp.f = 0.0f;
                            MOTOR_CS_RESET();
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                            MOTOR_CS_SET();
                    }
                    if (g_reg_write_pending) {
                        if(g_reg_write_addr == REG_A_SPEED_TARGET){
                            g_reg_write_value.f = -g_reg_write_value.f;
                        }
                        MOTOR_CS_RESET();
                        motor_write_reg(g_reg_write_addr, g_reg_write_value);
                        MOTOR_CS_SET();
                        g_reg_write_pending = 0;
                    }
                    
                }else{
                    Element_Handle(&current_element,adc_normalized,pitch,roll,yaw);
                    switch(current_element){
                        case ELEMENT_STRAIGHT:
                            common_speed = cruising_speed;
                            break;
                        case ELEMENT_RING_LEFT_ENTRANCE:
                        case ELEMENT_RING_LEFT_PASS:
                        case ELEMENT_IN_RING:
                            common_speed = ring_speed;
                            break;
                        case ELEMENT_RING_RIGHT_ENTRANCE:
                        case ELEMENT_RING_RIGHT_PASS:
                            common_speed = ring_speed;
                            break;
                        case ELEMENT_WALL:
                            common_speed = wall_speed;
                            break;
                        default:
                          //  common_speed = 30.0f;
													break;
                    }
                    /* --- 自动模式: 处理延迟 PID 参数写入 --- */
                    if (g_reg_write_pending) {
                        MOTOR_CS_RESET();
                        motor_write_reg(g_reg_write_addr, g_reg_write_value);
                        MOTOR_CS_SET();
                        g_reg_write_pending = 0;
                    }

                    /* 风扇启动后等待 3s 再激活电机 */
                    if (g_auto_transition == 1) {
                        if ((xTaskGetTickCount() - g_auto_fan_tick) >= pdMS_TO_TICKS(3000)) {
                            g_auto_transition = 2;
                            inner_yaw_rate_pi.error_sum = 0.0f;
                        }
                    }

                    /* 仅在过渡完成后写入电机速度目标 */
                    if (g_auto_transition == 2) {
                        /* 缓启动: ramp common_speed */
                     //   g_ss_magnitude = SoftStart_Ramp(g_ss_magnitude, common_speed, g_ss_step, SS_STEP_DOWN); //只用于启动，不阻止跑动时的响应
                        speed_target_l.f = common_speed - differential_speed;
                        speed_target_r.f = -(common_speed + differential_speed);
                        speed_target_r_nagtive = -speed_target_r.f;

                        MOTOR_CS_RESET();
                        motor_write_reg(REG_A_SPEED_TARGET, speed_target_r);
                        motor_write_reg(REG_B_SPEED_TARGET, speed_target_l);
                        MOTOR_CS_SET();
                    }

                }
                if(adc_values[0]<10 && adc_values[1]<10 && adc_values[2]<10 && adc_values[3]<10){
                    /* 赛道丢失, 紧急刹车, 切回手动模式 */
                    tmp.f = 0.0f;
                    MOTOR_CS_RESET();
                    motor_write_reg(REG_A_SPEED_TARGET, tmp);
                    motor_write_reg(REG_B_SPEED_TARGET, tmp);
                    MOTOR_CS_SET();
                    g_reg_write_pending = 0;
                    g_drive_direction = DRIVE_STOP;
                    manual_control_flag = 1;
                    PWMA_Set(0);
                    g_auto_transition = 0;
                    g_ss_magnitude = 0.0f;  /* 重置缓启动 */
                    g_err_angle_slewed = 0.0f;     /* 重置Err_Angle带宽限制 */
                    lpf_reset(&g_lpf_outer, 0.0f);
                    g_pid_output_filtered = 0.0f;   /* 重置目标角速度 */
                    g_last_err_angle = 0.0f;       /* 重置微分状态 */
                    ma_reset(&g_d_ma, 0.0f);
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

    // g_ec_ma_count_desc.peri_addr = (uint32_t)&(pid_struct.ec_count); /* EC移动平均已移除 */
    /* 注册 ADC 调试变量 */
    Debug_Register("adc0", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[0]);
    Debug_Register("adc1", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[1]);
    Debug_Register("adc2", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[2]);
    Debug_Register("adc3", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[3]);
    Debug_Register("yaw", DBG_FLOAT | HIGH_FREQ_TYPE, &yaw);
    Debug_Register("ErrFil", DBG_FLOAT | HIGH_FREQ_TYPE, &err_angle_filtered);
    /* 注册电机调试变量 */
    // Debug_Register("L_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &g_left_speed_target);
    Debug_Register("L_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &(speed_target_l.f));
    Debug_Register("R_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &(speed_target_r_nagtive));
    Debug_Register("L_Act", DBG_FLOAT | HIGH_FREQ_TYPE, &g_left_speed_actual);
    // Debug_Register("L_Err", DBG_FLOAT | HIGH_FREQ_TYPE, &g_left_speed_error);
    // Debug_Register("R_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &g_right_speed_target);
    Debug_Register("R_Act", DBG_FLOAT | HIGH_FREQ_TYPE, &g_right_speed_actual);
    
  
    //  Debug_Register("FKp", DBG_FLOAT | HIGH_FREQ_TYPE, &pid_struct.Kp0);
   // Debug_Register("FKd", DBG_FLOAT | HIGH_FREQ_TYPE, &pid_struct.Kd0);
  //  Debug_Register("FKi", DBG_FLOAT | HIGH_FREQ_TYPE, &pid_struct.Ki0);
    // Debug_Register("Mech",  DBG_UINT32 | HIGH_FREQ_TYPE, &g_mech_state);
    g_stream_active = 0;

    /* 初始化低通滤波器, 截止频率可在此修改 */
    lpf_init_fc(&g_lpf_left,  400.0f, 0.002f);  /* fc=20Hz, dt=2ms(500Hz) */
    lpf_init_fc(&g_lpf_right, 400.0f, 0.002f);
    lpf_init_fc(&g_lpf_outer, g_outer_lpf_fc, 0.002f);  /* 外环 fc=100Hz, dt=2ms */
    ma_init(&g_d_ma, g_d_ma_window);                     /* 微分MA滤波器, 默认窗口0=旁路 */

    /* 注册 PID 参数 (外设静态变量, 上位机按需读写) */
    Debug_Register("CH_Induct",  DBG_UINT8 | STATIC_TYPE , &change_inductor);
    Debug_Register("Err_K_A",   DBG_FLOAT | STATIC_TYPE , &Err_A_K);
    Debug_Register("Err_K_B",   DBG_FLOAT | STATIC_TYPE , &Err_B_K);
    Debug_Register("Err_K_C",   DBG_FLOAT | STATIC_TYPE , &Err_C_K);
    Debug_Register("Err_K_D",   DBG_FLOAT | STATIC_TYPE , &Err_D_K);
    Debug_Register("ADC_DeadZone", DBG_UINT16 | STATIC_TYPE, &adc_dead_zone);
    Debug_Register("ADC_ErrZone", DBG_FLOAT | STATIC_TYPE, &adc_err_zone);
    Debug_Register("Err_Angle", DBG_FLOAT | HIGH_FREQ_TYPE , &Err_Angle);
    //Debug_Register("Common_Spd", DBG_FLOAT | STATIC_TYPE, &common_speed);
    Debug_Register("Manual", DBG_UINT8 | STATIC_TYPE, &manual_control_flag);
    // Debug_Register("A_Kp",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_kp_desc);
    // Debug_Register("A_Ki",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_ki_desc);
    // Debug_Register("A_Kd",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_kd_desc);

   // Debug_Register("pid_output_k", DBG_FLOAT | STATIC_TYPE, &pid_output_k);
    Debug_Register("Fan_D", DBG_UINT16 | STATIC_TYPE | PERI_TYPE , &g_fan_d);
    Debug_Register("Drive_Spd", DBG_FLOAT | STATIC_TYPE, &drive_speed);
    Debug_Register("SS_Step",   DBG_FLOAT | STATIC_TYPE, &g_ss_step);
    Debug_Register("FKp0",      DBG_FLOAT | STATIC_TYPE, &pid_struct.Kp0);
    Debug_Register("Fe_max",    DBG_FLOAT | STATIC_TYPE, &pid_struct.e_max);
    // Debug_Register("Fsetpoint", DBG_FLOAT | STATIC_TYPE, &pid_struct.setpoint);

    /* 内环 PI 控制器参数 (静态, 上位机在线调参) */
    Debug_Register("Yaw_Kp",    DBG_FLOAT | STATIC_TYPE, &inner_yaw_rate_pi.Kp);
    Debug_Register("Yaw_Ki",    DBG_FLOAT | STATIC_TYPE, &inner_yaw_rate_pi.Ki);
    Debug_Register("YawLimit",DBG_FLOAT | STATIC_TYPE,&inner_yaw_rate_pi.error_sum_max);
    Debug_Register("YawHiLi",DBG_FLOAT|STATIC_TYPE,&inner_yaw_rate_pi.max_sep);
    Debug_Register("YawLoLi",DBG_FLOAT|STATIC_TYPE,&inner_yaw_rate_pi.min_sep);
    /* 外环模糊 P 输出 = 目标角速度, 内环实际角速度 (高频, 观察跟随效果) */
    Debug_Register("TgtYaw",    DBG_FLOAT | HIGH_FREQ_TYPE, &pid_output);
    Debug_Register("TgtYawF",   DBG_FLOAT | HIGH_FREQ_TYPE, &g_pid_output_filtered);
    Debug_Register("OutSlew",   DBG_FLOAT | STATIC_TYPE, &g_outer_slew_max);
    Debug_Register("OutLPFFc",  DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_outer_lpf_fc_desc);
    Debug_Register("D_MA_Win",  DBG_UINT16 | STATIC_TYPE | PERI_TYPE, &g_d_ma_win_desc);
    Debug_Register("Kd_Gyro",   DBG_FLOAT | STATIC_TYPE, &g_Kd_gyro);
    Debug_Register("KdGyroHi",  DBG_FLOAT | STATIC_TYPE, &g_Kd_gyro_high);
    Debug_Register("KdGyroLo",  DBG_FLOAT | STATIC_TYPE, &g_Kd_gyro_low);
    Debug_Register("KdGyroTh",  DBG_FLOAT | STATIC_TYPE, &g_Kd_gyro_thresh);
    Debug_Register("Kd_Diff",   DBG_FLOAT | STATIC_TYPE, &g_Kd_diff);
    Debug_Register("DiffDead",  DBG_FLOAT | STATIC_TYPE, &g_diff_deadband);
    Debug_Register("FTerm",     DBG_FLOAT | HIGH_FREQ_TYPE, &g_filtered_d_term);
    Debug_Register("TarYawR",   DBG_FLOAT | HIGH_FREQ_TYPE, &g_tar_yaw_raw);
    Debug_Register("ActYaw",    DBG_FLOAT | HIGH_FREQ_TYPE, &actual_yaw_rate_debug);

    Debug_Register("ComSpd", DBG_FLOAT | STATIC_TYPE, &common_speed);

    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NB]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NM]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NS]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_ZO]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PS]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PM]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PB]);
    
    Debug_Register("A_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_ilim_desc);
    // Debug_Register("B_Kp",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_kp_desc);
    // Debug_Register("B_Ki",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_ki_desc);
    // Debug_Register("B_Kd",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_kd_desc);
     Debug_Register("B_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_ilim_desc);
   // Debug_Register("FKi0", DBG_FLOAT | STATIC_TYPE, &pid_struct.Ki0);
    Debug_Register("NormK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[0]);
    Debug_Register("NormK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[1]);
    Debug_Register("NormK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[2]);
    Debug_Register("NormK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[3]);

    Debug_Register("stat",DBG_UINT8,&current_element);

    Debug_Register("Cruising",DBG_FLOAT | STATIC_TYPE,&cruising_speed);
    Debug_Register("RingSpd",DBG_FLOAT | STATIC_TYPE,&ring_speed);
    Debug_Register("WallSpd",DBG_FLOAT | STATIC_TYPE,&wall_speed);
    Debug_Register("Barrel",DBG_FLOAT | STATIC_TYPE,&barrel_speed);

    /* 开环过元素参数见 element.c: OL_Ang, OL_Yaw, PassTmOut, RingSide */
    /* 初始化 SPI3, ADC */
    SPI3_Init(SPI3_Callback);
    ADC2_DMA_Init(adc_dma_buf);
    ADC2_DMA_Start();

    /* 内环 PI 控制器初始参数 (上位机可在线调参) */
    inner_yaw_rate_pi.Kp = 0.15000000596046448f;
    inner_yaw_rate_pi.Ki = 0.0003000000142492354f;
    inner_yaw_rate_pi.error_sum = 0.0f;
    inner_yaw_rate_pi.error_sum_max = 50000.0f;
    pid_struct.e_max = 150.0f; /* 根据实际误差范围调整 */
    pid_struct.Kp0 = 0.17f; /* 基础 Kp */
    pid_struct.setpoint = 0.0f; /* 目标值 (循迹一般设为0) */
    FuzzyPID_Init(&pid_struct, pid_struct.Kp0, pid_struct.e_max);
    /* 覆盖模糊 Kp 单点值 (FKp NB~PB) */
    pid_struct.kp_singletons[FUZZY_NB] = 1.2000000476837158f;
    pid_struct.kp_singletons[FUZZY_NM] = 1.0f;
    pid_struct.kp_singletons[FUZZY_NS] = 1.0f;
    pid_struct.kp_singletons[FUZZY_ZO] = 1.0f;
    pid_struct.kp_singletons[FUZZY_PS] = 1.0f;
    pid_struct.kp_singletons[FUZZY_PM] = 1.0f;
    pid_struct.kp_singletons[FUZZY_PB] = 1.2000000476837158f;
    //vTaskDelay(pdMS_TO_TICKS(10));
    
    /* 创建 Main_Control_Task (低优先级, 2ms 周期) */
     xTaskCreate(Main_Control_Task, "main_control", 1224, NULL, tskIDLE_PRIORITY + 3, &xMainControlTaskHandle);
}
