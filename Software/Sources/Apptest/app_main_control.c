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

/* ================================================================
 * 速度目标值宏 (临时占位, 后续调整)
 * ================================================================ */
#define LEFT_SPEED_TARGET   5.0f
#define RIGHT_SPEED_TARGET  -5.0f

/* ================================================================
 * TaskNotify 信号位定义
 * ================================================================ */
#define SIGNAL_MOTOR_DATA_READY  (1 << 3)

/* ================================================================
 * 全局变量
 * ================================================================ */

/* ADC DMA 缓冲区 */
static uint8_t xdata adc_dma_buf[ADC2_DMASIZE];

/* ADC 各通道采样值 */
static uint16_t adc_values[ADC2_CH_NUM];

/* 电机数据 (500Hz stream 推送) */
static float    g_left_speed_actual;
static float    g_right_speed_actual;
static uint32_t g_mech_state;

/* 速度目标 & 误差 */
static motor_data_t g_left_speed_target;
static motor_data_t g_right_speed_target;
static float g_left_speed_error;
static float g_right_speed_error;

/* PID 参数访问互斥锁 (非0=正在进行PID读写, Main_Control需跳过电机写入) */
static volatile uint8_t g_pid_access_locked = 0;

/* Stream 模式状态位 (0=已关闭, 1=已开启) */
static volatile uint8_t g_stream_active = 0;

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
    return;
}

static void Motor_Reg_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    float f;
    motor_data_t val;
    (void)len;
    memcpy(&f, pData, sizeof(float));
    val.f = f;

    MOTOR_CS_RESET();
    motor_write_reg((uint8_t)peri_addr, val);
    MOTOR_CS_SET();
   
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
void Main_Control_Task(void *pvParameters)
{
    uint8_t i;
    motor_data_t tmp;
    uint32_t ulNotifiedValue;
    
    (void)pvParameters;

    tmp.f = LEFT_SPEED_TARGET;
    g_left_speed_target = tmp;
    tmp.f = RIGHT_SPEED_TARGET;
    g_right_speed_target = tmp;

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
        if (xTaskNotifyWait(0, SIGNAL_MOTOR_DATA_READY, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
            if (ulNotifiedValue & SIGNAL_MOTOR_DATA_READY) {
                    

            /* --- 2. 写入左右电机速度目标 (stream 关闭或 PID 访问期间跳过) --- */
            if (g_stream_active) {
                taskENTER_CRITICAL();
                MOTOR_CS_RESET();
                for(i=0;i<12;i++){
                    spi_buffer[i] = MOTOR_SPI_TxRx(0xFFu);
                }
                
                tmp=motor_spi_dat_handle(spi_buffer);
                g_left_speed_actual = tmp.f;
                tmp=motor_spi_dat_handle(spi_buffer+4);
                g_right_speed_actual = tmp.f;
                tmp = motor_spi_dat_handle(spi_buffer+8);
                g_mech_state = tmp.u32;
                MOTOR_CS_SET();
                NOP10();
                MOTOR_CS_RESET();
                motor_write_reg(REG_A_SPEED_TARGET, g_right_speed_target);
                motor_write_reg(REG_B_SPEED_TARGET, g_left_speed_target);
                MOTOR_CS_SET();
               
                taskEXIT_CRITICAL();

               
                
            }
			if (ADC2_DMA_IsDone()) {
                ADC2_DMA_ClearFlag();
                for (i = 0; i < ADC2_CH_NUM; i++) {
                    adc_values[i] = ADC2_GetAvgValue(i);
                }
                //ADC2_DMA_Start();
            }
            
            }
        }
    
       // vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
       // vTaskDelay(pdMS_TO_TICKS(2)); /* 额外延时 2ms, 避免与电机数据读取任务过于接近导致总线冲突 */
        /* --- 1. ADC 采样处理 --- */
        
       
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

    /* 注册 ADC 调试变量 */
    Debug_Register("adc0", DBG_UINT8 | HIGH_FREQ_TYPE, &spi_buffer[0]);
    Debug_Register("adc1", DBG_UINT8 | HIGH_FREQ_TYPE, &spi_buffer[1]);
    Debug_Register("adc2", DBG_UINT8 | HIGH_FREQ_TYPE, &spi_buffer[2]);
    Debug_Register("adc3", DBG_UINT8 | HIGH_FREQ_TYPE, &spi_buffer[3]);

    /* 注册电机调试变量 */
    // Debug_Register("L_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &g_left_speed_target);
     Debug_Register("L_Act", DBG_FLOAT | HIGH_FREQ_TYPE, &g_left_speed_actual);
    // Debug_Register("L_Err", DBG_FLOAT | HIGH_FREQ_TYPE, &g_left_speed_error);
    // Debug_Register("R_Tar", DBG_FLOAT | HIGH_FREQ_TYPE, &g_right_speed_target);
     Debug_Register("R_Act", DBG_FLOAT | HIGH_FREQ_TYPE, &g_right_speed_actual);
    // Debug_Register("R_Err", DBG_FLOAT | HIGH_FREQ_TYPE, &g_right_speed_error);
    // Debug_Register("Mech",  DBG_UINT32 | HIGH_FREQ_TYPE, &g_mech_state);
    g_stream_active = 0;
    /* 注册 PID 参数 (外设静态变量, 上位机按需读写) */
    // Debug_Register("A_Kp",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_kp_desc);
    // Debug_Register("A_Ki",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_ki_desc);
    // Debug_Register("A_Kd",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_kd_desc);
    // Debug_Register("A_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_a_ilim_desc);
    // Debug_Register("B_Kp",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_kp_desc);
    // Debug_Register("B_Ki",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_ki_desc);
    // Debug_Register("B_Kd",   DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_kd_desc);
    // Debug_Register("B_ILim", DBG_FLOAT | STATIC_TYPE | PERI_TYPE, &g_pid_b_ilim_desc);

    /* 初始化 SPI3, ADC */
    SPI3_Init(SPI3_Callback);
    ADC2_DMA_Init(adc_dma_buf);
    ADC2_DMA_Start();
   // vTaskDelay(pdMS_TO_TICKS(10));
    
     /* 创建 Motor_Data_Task (高优先级, 500Hz 触发) */
    /* 创建 Motor_Data_Task (高优先级, 500Hz 触发) */
  //  xTaskCreate(Motor_Data_Task, "motor_data", 512, NULL, tskIDLE_PRIORITY + 1, &xMotorDataTaskHandle);

    /* 创建 Main_Control_Task (低优先级, 2ms 周期) */
    xTaskCreate(Main_Control_Task, "main_control", 1024, NULL, tskIDLE_PRIORITY + 3, &xMainControlTaskHandle);
}
