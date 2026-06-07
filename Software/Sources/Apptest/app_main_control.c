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
#include "fuzzy_pid.h"

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

/* PID 参数访问互斥锁 (非0=正在进行PID读写, Main_Control需跳过电机写入) */
static volatile uint8_t g_pid_access_locked = 0;

/* PID 参数延迟写入 (回调只置标志, Main_Control 安全点执行 SPI 写入, 避免竞态) */
static volatile uint8_t g_reg_write_pending = 0;
static uint8_t          g_reg_write_addr;
static motor_data_t     g_reg_write_value;

/* 手动模式超时保护 (3s 无指令自动刹车) */
static TickType_t g_last_manual_cmd_tick = 0;
static uint8_t    g_manual_cmd_received  = 0;

/* Stream 模式状态位 (0=已关闭, 1=已开启) */
static volatile uint8_t g_stream_active = 0;

/* 自动模式过渡状态: 0=空闲, 1=风扇已写入等待3s, 2=电机已激活 */
static uint8_t      g_auto_transition = 0;
static TickType_t   g_auto_fan_tick   = 0;



/* 任务句柄 */
TaskHandle_t xMotorDataTaskHandle    = NULL;
TaskHandle_t xMainControlTaskHandle = NULL;

/* ================================================================
 * SPI3 DMA 回调 (预留)
 * ================================================================ */
void SPI3_Callback(void *pvParameters)
{
    (void)pvParameters;
}

/* ================================================================
 * 电机寄存器 读/写 回调 (供 Debug 系统 PERI_TYPE 使用)
 *   上位机请求 STATIC_REFRESH 时触发读, 写变量时触发写
 * ================================================================ */
static void Motor_Reg_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    float tmp;
    tmp = 0.0f;
    memcpy(pData,&tmp,sizeof(float));
    return;
}

static void Motor_Reg_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    (void)len;
    memcpy(&g_reg_write_value.u32, pData, sizeof(uint32_t));
    g_reg_write_addr = (uint8_t)peri_addr;
    g_reg_write_pending = 1;
    g_last_manual_cmd_tick = xTaskGetTickCount();
    g_manual_cmd_received = 1;
}

/*用于将速度和pid解耦的结构体*/
#define TRACK_WIDTH 11.0f
typedef struct speed_controller{
    float radius;
    float straight_speed;
    float controller_limit;
    float controller_k;
} speed_controller_t;
void update_speed_controller(speed_controller_t* controller){
   controller->controller_k = (controller->straight_speed * TRACK_WIDTH) / 
   (2.0f * controller->radius * 2 * controller->controller_limit * 4.0f / 5.0f);
}
speed_controller_t g_speed_controller;

static void Speed_Controller_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
        (void)peri_addr;
        (void)len;
        memcpy(pData, (void*)(peri_addr), sizeof(float));
    return;
}

static void Speed_Controller_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    (void)peri_addr;
    (void)len;
    memcpy((void*)(peri_addr), pData, sizeof(float));
    update_speed_controller(&g_speed_controller);
    return;
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
    if (count > FUZZY_EC_MA_MAX) count = FUZZY_EC_MA_MAX;
    *(char *)(peri_addr) = (char)count;
}
static float normalized_adc_k[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // ADC 归一化系数数组
static uint8_t normalize_index = 0;
static uint8_t adc_start_normalize = 0;
static void NormalizeADC(uint32_t peri_addr,void *pData,uint8_t len){
    uint8_t* ptr = (uint8_t *)pData;
    (void)peri_addr;
    (void)len;
    if(*ptr<4){
        normalize_index = *ptr;
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
/* PID 参数外设描述符 (Motor A / 左电机) */
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
static DebugPeriDesc_t g_fan_voltage_desc  = {REG_FAN_VOLTAGE, Motor_Reg_Read_Cb, Motor_Reg_Write_Cb}; // 风扇控制寄存器
static DebugPeriDesc_t g_speed_controller_limit_desc = {0, Speed_Controller_Read_Cb, Speed_Controller_Write_Cb}; /* 速度控制器参数 (非寄存器映射) */
static DebugPeriDesc_t g_speed_controller_radius_desc = {0, Speed_Controller_Read_Cb, Speed_Controller_Write_Cb};
static DebugPeriDesc_t g_speed_controller_straight_speed_desc = {0, Speed_Controller_Read_Cb, Speed_Controller_Write_Cb};
static DebugPeriDesc_t g_ec_ma_count_desc = {0, EC_MA_Read_Cb, EC_MA_Write_Cb}; /* EC移动平均窗口大小 */
/* ================================================================
 * Motor_Data_Task
 *   由 P2.0 中断 (500Hz) 通过 TaskNotify 触发
 *   连续读取 3 个 32bit 值: 左速 / 右速 / 机械状态
 * ================================================================ */

void Motor_Data_Task(void *pvParameters)
{
    motor_data_t dat;
    uint32_t ulNotifiedValue;
    (void)pvParameters;

    for (;;) {
        if (xTaskNotifyWait(0, SIGNAL_MOTOR_DATA_READY, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
            if (ulNotifiedValue & SIGNAL_MOTOR_DATA_READY) {
       

            
            }
        }
    }
}
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

float Err_A_K=120.0f;  //所有adc 标定值为2200左右
float Err_B_K=120.0f;
float Err_C_K=3.0f;
float Err_D_K=2.0f;
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

static FuzzyPID pid_struct={0.0f};
static float pid_output=0.0f;
static float common_speed=30.0f;
static float pid_output_k = 0.10f;
static motor_data_t speed_target_l={0};
static motor_data_t speed_target_r={0};
static float speed_target_r_nagtive=0.0f;
static float numerator, denominator;
static float err_max = 120.0f;
#define spd_limit 150.0f
void Change_Maunal(){
    motor_data_t tmp;
	if(manual_control_flag){
        /* 切换到自动模式: 先通过 Main_Control_Task 异步写入风扇,
         * 风扇启动 3s 后再激活电机 */
        manual_control_flag = 0;
        g_auto_transition = 0;  /* 风扇尚未写入 */
        g_reg_write_value.f = 2.5f;
        g_reg_write_addr = REG_FAN_VOLTAGE;
        g_reg_write_pending = 1;
    }else{
        manual_control_flag=1;
        tmp.f = 0.0f;
        g_auto_transition = 0;  /* 风扇尚未写入 */
        g_reg_write_value.f = 0.0f;
        g_reg_write_addr = REG_FAN_VOLTAGE;
        g_reg_write_pending = 1;
      //  Debug_Log("Manual Mode");
    }
}

extern void IMU_Get_Angle(float* roll, float* pitch, float* yaw);
extern float IMU_Get_Yaw_Unwrapped(void);
static float sqrt_negative(float value) {

    if (value < 0.0f) {
        return -sqrt(-value);
    } else {
        return sqrt(value);
    }
}
static uint8_t change_inductor = 0;
static float roll=0.0f, pitch=0.0f, yaw=0.0f;
static float adc_avg_sum = 0.0f;
static uint16_t adc_avg = 0;
static uint16_t adc_avg_count = 0;
static uint8_t display_adc_original = 0;
static float adc_normalized[4] = {0.0f};
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
    float gyro_z;
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
               // IMU_Get_Angle(&roll, &pitch, &yaw);
                yaw = IMU_Get_Yaw_Unwrapped();
                gyro_z = IMU_Filter_Data->sensor.dat.gz - IMU_Filter_Data->bias_gz;
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
                        if(adc_avg > 4000){
                            normalized_adc_k[normalize_index] = 1000.0f / (float)adc_avg; // 简单线性归一化
                        }else{
                            normalized_adc_k[normalize_index] = 1.0f; // 不进行归一化
                        }
                        adc_start_normalize = 0;
                    }
                }
                if(adc_values[0]<adc_dead_zone) adc_values[0]=0;
                if(adc_values[1]<adc_dead_zone) adc_values[1]=0;
                if(adc_values[2]<adc_dead_zone) adc_values[2]=0;
                if(adc_values[3]<adc_dead_zone) adc_values[3]=0;
                if(display_adc_original){
                    adc_normalized[0] = adc_values[0];
                    adc_normalized[1] = adc_values[1];
                    adc_normalized[2] = adc_values[2];
                    adc_normalized[3] = adc_values[3];
                }else{
                    adc_normalized[0] = adc_values[0] * normalized_adc_k[0];
                    adc_normalized[1] = adc_values[1] * normalized_adc_k[1];
                    adc_normalized[2] = adc_values[2] * normalized_adc_k[2 ];
                    adc_normalized[3] = adc_values[3] * normalized_adc_k[3];
                }
       
            
                if(change_inductor == 1){
                    Err_1 = (float)(sqrt(adc_normalized[0]) - sqrt(adc_normalized[2]));
                    Err_2 = (float)(sqrt(adc_normalized[1]) - sqrt(adc_normalized[3]));
                    Err_3 = (float)(adc_normalized[0] + adc_normalized[2]);
                    Err_4 = (float)(adc_normalized[1] + adc_normalized[3]);
                }else{
                    Err_1 = (float)(sqrt(adc_normalized[1]) - sqrt(adc_normalized[3]));
                    Err_2 = (float)(sqrt(adc_normalized[0]) - sqrt(adc_normalized[2]));
                    Err_3 = (float)(adc_normalized[1] + adc_normalized[3]);
                    Err_4 = (float)(adc_normalized[0] + adc_normalized[2]);
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
                pid_output = FuzzyPID_Step(&pid_struct, Err_Angle, gyro_z);

                speed_target_l.f = -pid_output*g_speed_controller.controller_k + g_speed_controller.straight_speed;
                speed_target_r.f = -(pid_output*g_speed_controller.controller_k + g_speed_controller.straight_speed);
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
                    }

                    /* 执行驾驶指令 (标志位 -> 电机写入) */
                    if (g_drive_direction != DRIVE_STOP) {
                        MOTOR_CS_RESET();
                        if (g_drive_direction == DRIVE_FORWARD) {
                            tmp.f = -drive_speed;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            tmp.f = drive_speed;
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        } else if (g_drive_direction == DRIVE_BACKWARD) {
                            tmp.f = drive_speed;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            tmp.f = -drive_speed;
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        } else if (g_drive_direction == DRIVE_LEFT) {
                            tmp.f = -drive_speed;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            tmp.f = -drive_speed;
                            motor_write_reg(REG_B_SPEED_TARGET, tmp);
                        } else if (g_drive_direction == DRIVE_RIGHT) {
                            tmp.f = drive_speed;
                            motor_write_reg(REG_A_SPEED_TARGET, tmp);
                            tmp.f = drive_speed;
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
                    
                    /* --- 自动模式: 处理延迟写入 --- */
                    if (g_reg_write_pending) {
                        MOTOR_CS_RESET();
                        motor_write_reg(g_reg_write_addr, g_reg_write_value);
                        MOTOR_CS_SET();
                        g_reg_write_pending = 0;

                        if (g_reg_write_addr == REG_FAN_VOLTAGE) {
                            /* 风扇已启动, 记录时刻, 进入 3s 等待 */
                            g_auto_transition = 1;
                            g_auto_fan_tick = xTaskGetTickCount();
                        }
                    }

                    /* 风扇启动后等待 3s 再激活电机 */
                    if (g_auto_transition == 1) {
                        if ((xTaskGetTickCount() - g_auto_fan_tick) >= pdMS_TO_TICKS(3000)) {
                            g_auto_transition = 2;
                            pid_struct.integral = 0.0f;
                        }
                    }

                    /* 仅在过渡完成后写入电机速度目标 */
                    if (g_auto_transition == 2) {
                       
                        MOTOR_CS_RESET();
                        motor_write_reg(REG_A_SPEED_TARGET, speed_target_r);
                        motor_write_reg(REG_B_SPEED_TARGET, speed_target_l);
                        MOTOR_CS_SET();
                    }

                }
                if(adc_values[0]<10 && adc_values[1]<10 && adc_values[2]<10 && adc_values[3]<10){
                    /* 赛道丢失, 紧急刹车 */
                    tmp.f = 0.0f;
                    MOTOR_CS_RESET();
                    motor_write_reg(REG_A_SPEED_TARGET, tmp);
                    motor_write_reg(REG_B_SPEED_TARGET, tmp);
                    MOTOR_CS_SET();
                    g_reg_write_pending = 0;
                    g_drive_direction = DRIVE_STOP;
                    manual_control_flag=1;
                   // tmp.f = 0.0f;
                    g_auto_transition = 0;  /* 风扇尚未写入 */
                    g_reg_write_value.f = 0.0f;
                    g_reg_write_addr = REG_FAN_VOLTAGE;
                    g_reg_write_pending = 1;
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
 *   通知 Motor_Data_Task
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

    g_speed_controller_limit_desc.peri_addr = (uint32_t)&(g_speed_controller.controller_limit);
    g_speed_controller_radius_desc.peri_addr = (uint32_t)&(g_speed_controller.radius);
    g_speed_controller_straight_speed_desc.peri_addr = (uint32_t)&(g_speed_controller.straight_speed);
    g_ec_ma_count_desc.peri_addr = (uint32_t)&(pid_struct.ec_count); /* EC 移动平均窗口大小, 供上位机调整 */
    /* 注册 ADC 调试变量 */
    Debug_Register("adc0", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[0]);
    Debug_Register("adc1", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[1]);
    Debug_Register("adc2", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[2]);
    Debug_Register("adc3", DBG_FLOAT | HIGH_FREQ_TYPE, &adc_normalized[3]);
    Debug_Register("yaw", DBG_FLOAT | HIGH_FREQ_TYPE, &yaw);
    Debug_Register("pid_output", DBG_FLOAT | HIGH_FREQ_TYPE, &pid_output);
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
    Debug_Register("ec",DBG_FLOAT | HIGH_FREQ_TYPE, &pid_struct.temp_ec);
    // Debug_Register("A_Kp",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_kp_desc);
    // Debug_Register("A_Ki",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_ki_desc);
    // Debug_Register("A_Kd",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_kd_desc);

   // Debug_Register("pid_output_k", DBG_FLOAT | STATIC_TYPE, &pid_output_k);
    Debug_Register("Fan_Voltage", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_fan_voltage_desc);
    Debug_Register("Drive_Spd", DBG_FLOAT | STATIC_TYPE, &drive_speed);
    Debug_Register("FKp0",      DBG_FLOAT | STATIC_TYPE, &pid_struct.Kp0);
    Debug_Register("FKi0",      DBG_FLOAT | STATIC_TYPE, &pid_struct.Ki0);
    Debug_Register("FKd0",      DBG_FLOAT | STATIC_TYPE, &pid_struct.Kd0);
    Debug_Register("EC_DeadZone", DBG_FLOAT | STATIC_TYPE, &pid_struct.ec_dead_zone);
    Debug_Register("EC_Avg",   DBG_UINT16 | STATIC_TYPE | PERI_TYPE, &g_ec_ma_count_desc);
    Debug_Register("EC_Thresh_L", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_err_thresh_l);
    Debug_Register("EC_Thresh_R", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_err_thresh_r);
    Debug_Register("Fe_max",    DBG_FLOAT | STATIC_TYPE, &pid_struct.e_max);
    Debug_Register("Fec_max",   DBG_FLOAT | STATIC_TYPE, &pid_struct.ec_max);
    Debug_Register("Ferr_thr",  DBG_FLOAT | STATIC_TYPE, &pid_struct.err_thresh);
    Debug_Register("Ferr_clear",DBG_FLOAT | STATIC_TYPE, &pid_struct.err_clear_thresh);
   // Debug_Register("Fintegral", DBG_FLOAT | STATIC_TYPE, &pid_struct.integral);
   // Debug_Register("Fsetpoint", DBG_FLOAT | STATIC_TYPE, &pid_struct.setpoint);
    Debug_Register("Flimit",    DBG_FLOAT | STATIC_TYPE, &pid_struct.limit);
    Debug_Register("K_gyro",    DBG_FLOAT | STATIC_TYPE, &pid_struct.K_gyro);
   // Debug_Register("Fprev_err", DBG_FLOAT | STATIC_TYPE, &pid_struct.prev_error);
    Debug_Register("Crtl_Limit", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &(g_speed_controller_limit_desc));
    Debug_Register("Crtl_Radius", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &(g_speed_controller_radius_desc));
    Debug_Register("Crtl_ComSpd", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &(g_speed_controller_straight_speed_desc));

    Debug_Register("ComSpd", DBG_FLOAT | STATIC_TYPE, &g_speed_controller.straight_speed);
    Debug_Register("Crtl_K", DBG_FLOAT | STATIC_TYPE, &g_speed_controller.controller_k);

    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NB]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NM]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_NS]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_ZO]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PS]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PM]);
    Debug_Register("FKp", DBG_FLOAT | STATIC_TYPE, &pid_struct.kp_singletons[FUZZY_PB]);

    Debug_Register("FKd", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_singletons[FUZZY_NB]);
    Debug_Register("FKd", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_singletons[FUZZY_NM]);
    Debug_Register("FKd", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_singletons[FUZZY_NS]);
    Debug_Register("FKd", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_singletons[FUZZY_ZO]);
    Debug_Register("FKd", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_singletons[FUZZY_PS]);
    Debug_Register("FKd", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_singletons[FUZZY_PM]);
    Debug_Register("FKd", DBG_FLOAT | STATIC_TYPE, &pid_struct.kd_singletons[FUZZY_PB]);

         Debug_Register("A_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_ilim_desc);
    // Debug_Register("B_Kp",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_kp_desc);
    // Debug_Register("B_Ki",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_ki_desc);
    // Debug_Register("B_Kd",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_kd_desc);
     Debug_Register("B_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_ilim_desc);
   // Debug_Register("FKi0", DBG_FLOAT | STATIC_TYPE, &pid_struct.Ki0);
    Debug_Register("NormalizedK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[0]);
    Debug_Register("NormalizedK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[1]);
    Debug_Register("NormalizedK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[2]);
    Debug_Register("NormalizedK", DBG_FLOAT| STATIC_TYPE, &normalized_adc_k[3]);
    /* 初始化 SPI3, ADC */
    SPI3_Init(SPI3_Callback);
    ADC2_DMA_Init(adc_dma_buf);
    ADC2_DMA_Start();

    g_speed_controller.radius = 5.0f;
    g_speed_controller.straight_speed = 20.0f;
    g_speed_controller.controller_limit = 330.0f;
    update_speed_controller(&g_speed_controller);
    pid_struct.e_max = 150.0f; /* 根据实际误差范围调整 */
    pid_struct.ec_max = 1200.0f; /* 根据实际误差变化率 */
    pid_struct.Kp0 = 2.0f; /* 基础 Kp */
    pid_struct.Ki0 = 30.0f; /* 基础 Ki */
    pid_struct.Kd0 = 0.0f; /* 基础 Kd */
    pid_struct.err_thresh = 150.0f; /* 积分分离阈值 */
    pid_struct.integral = 0.0f;
    pid_struct.setpoint = 0.0f; /* 目标值 (循迹一般设为0) */
    pid_struct.prev_error = 0.0f;
    pid_struct.kd_err_thresh_l = 50.0f;
    pid_struct.kd_err_thresh_r = 50.0f;
    FuzzyPID_Init(&pid_struct, pid_struct.Kp0, pid_struct.Ki0, pid_struct.Kd0, pid_struct.e_max, pid_struct.ec_max, pid_struct.err_thresh);
    pid_struct.ec_dead_zone = 50.0f;
    pid_struct.ec_count = 40;
    pid_struct.limit = 100.0f;
    pid_struct.K_gyro = -40.0f;
    /* 覆盖模糊 Kp 单点值 (FKp NB~PB) */
    pid_struct.kp_singletons[FUZZY_NB] = 1.6f;
    pid_struct.kp_singletons[FUZZY_NM] = 1.5f;
    pid_struct.kp_singletons[FUZZY_NS] = 1.3f;
    pid_struct.kp_singletons[FUZZY_ZO] = 1.0f;
    pid_struct.kp_singletons[FUZZY_PS] = 1.3f;
    pid_struct.kp_singletons[FUZZY_PM] = 1.5f;
    pid_struct.kp_singletons[FUZZY_PB] = 1.6f;
    //vTaskDelay(pdMS_TO_TICKS(10));
    
     /* 创建 Motor_Data_Task (高优先级, 500Hz 触发) */
    /* 创建 Motor_Data_Task (高优先级, 500Hz 触发) */
  //  xTaskCreate(Motor_Data_Task, "motor_data", 512, NULL, tskIDLE_PRIORITY + 1, &xMotorDataTaskHandle);
    
    /* 创建 Main_Control_Task (低优先级, 2ms 周期) */
     xTaskCreate(Main_Control_Task, "main_control", 1224, NULL, tskIDLE_PRIORITY + 3, &xMainControlTaskHandle);
}
