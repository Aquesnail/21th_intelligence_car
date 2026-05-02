#include "icm42688.h"
#include "icm42688_defs.h"
#include "spi2_user.h"
#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "imu_filter.h"
#include "debug_arch.h"
#include "storage_port.h"
#include <stdarg.h>
#include "timer.h"


#pragma warning disable = 138
#pragma warning disable = 151

#define FIFO_Packets 2

#define SIGNAL_IMU_DATA_READY  (1 << 0)  // 位 0：外部中断引脚触发
#define SIGNAL_SPI_DMA_DONE    (1 << 1)  // 位 1：DMA 传输完成 //任务通知比信号量快
/* =======================================================
 * 外部变量定义（供icm42688.c使用）
 * ======================================================= */
float   icm_acc_sensitivity = 0.0f;      /* 加速度灵敏度 */
float   icm_gyro_sensitivity = 0.0f;     /* 陀螺仪灵敏度 */
uint8_t icm_current_bank = 0;         /* 当前寄存器bank */
uint16_t icm_fifo_ths = 0;            /* FIFO阈值 */

SemaphoreHandle_t xDmaSemaphore = NULL;
TaskHandle_t   xImuTaskHandle = NULL;
static uint8_t xdata FIFO_RxBuffer[FIFO_Packets*16 +1] = {0};
static uint8_t xdata FIFO_TxBuffer[FIFO_Packets*16 +1] = {0};
IMU_Filter_t *IMU_Filter_Data = NULL;
static ICM42688_Pack16_t IMU_Dat = {0};

extern SemaphoreHandle_t xDebugReadySignal;

/* =======================================================
 * 回调函数声明
 * ======================================================= */
void IMU_INT1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
void IMU_DMA_Read_CallBack(BaseType_t* xSPI_DmaSemaphore);

/* =======================================================
 * icm42688.h 要求的外部函数实现
 * ======================================================= */

/* SPI单字节收发 */
uint8_t ICM42688_SPI_TxRx(uint8_t dat) {
    uint8_t rx_dat;
    taskENTER_CRITICAL();
    rx_dat = SPI2_WriteReadByte(dat);
    taskEXIT_CRITICAL();
    return rx_dat;
}

/* SPI DMA批量读取 */
void ICM42688_SPI_DmaRead(uint8_t* rx_buf, uint16_t len) {
    uint32_t ulNotifiedValue;
    SPI2_DMA_WriteReadBytes_IT(FIFO_TxBuffer, rx_buf, len);
    if (xTaskNotifyWait(0, SIGNAL_SPI_DMA_DONE, &ulNotifiedValue, pdMS_TO_TICKS(10)) == pdTRUE) {
        return;
    } else {
        // 异常超时处理
    }
}

/* 毫秒延时 */
void ICM42688_DelayMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
    //vTaskDelay(pdMS_TO_TICKS(ms));
}

/* 调试打印 */
void ICM42688_Print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vDebug_Log(fmt, args); // 直接转发给底层函数，避免二次包装
    va_end(args);
}
/* =======================================================
 * 中断回调
 * ======================================================= */
void IMU_INT1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken) {
    xTaskNotifyFromISR(xImuTaskHandle, SIGNAL_IMU_DATA_READY, eSetBits, xHigherPriorityTaskWoken);
}

void IMU_DMA_Read_CallBack(BaseType_t*  xHigherPriorityTaskWoken) {
    xTaskNotifyFromISR(xImuTaskHandle, SIGNAL_SPI_DMA_DONE, eSetBits, xHigherPriorityTaskWoken);
}

// void TIMER9_TIMER10_ISR(void) interrupt TMR9_TMR10_VECTOR
// {
//     //<<AICUBE_USER_TIMER10_ISR_CODE1_BEGIN>>
//     // 在此添加中断函数用户代码  
//     if (TIMER10_CheckFlag())            //判断定时器10中断
//     {
//         P14=~P14; 
//         TIMER10_ClearFlag();            //清除定时器10中断标志
//     }
//     //<<AICUBE_USER_TIMER10_ISR_CODE1_END>>
// }

/* =======================================================
 * IMU任务
 * ======================================================= */
float acc_t[3] = {0.0f};
float gyro_t[3] = {0.0f};
float temp_t = 0.0f;


/* =======================================================
 * IMU 处理任务：基于 FSYNC 标记的斜率补偿与时间戳对齐
 * ======================================================= */
uint16_t delta_mcu_time=0;
uint32_t fsync_time=0;
uint32_t last_mcu_time=0;
uint32_t mcu_time=0;
uint16_t last_imu_time=0;
uint16_t delta_imu_time=0;
uint16_t current_imu_time=0;
uint8_t fsync_received=0;
float last_imu_sample_time=1000e-6;
float imu_slope_k=1.066;
float last_imu_slope_k = 1.066;
void vIMUProcessTask(void *pvParameters) {
    /* -----------------------------------------------------------
     * 1. 局部变量声明 (严格遵循 C89 标准，定义在函数开头)
     * ----------------------------------------------------------- */
    uint8_t xdata* ptr;
    uint16_t i;
    uint32_t ulNotifiedValue;
    uint8_t tt = 0;
    
    

    (void)pvParameters;

    /* -----------------------------------------------------------
     * 2. 初始化逻辑
     * ----------------------------------------------------------- */
    xImuTaskHandle = xTaskGetCurrentTaskHandle();

    /* 等待调试系统准备就绪 (如有) */
    if (xDebugReadySignal != NULL) {
        xSemaphoreTake(xDebugReadySignal, portMAX_DELAY);
        xSemaphoreGive(xDebugReadySignal);
    }

    memset(FIFO_TxBuffer, 0xff, sizeof(FIFO_TxBuffer));
    vTaskDelay(pdMS_TO_TICKS(2));

    SPI2_Init(IMU_DMA_Read_CallBack);
    icm_fifo_ths = FIFO_Packets; /* 每次读取 2 个包 */

    i = ICM42688_Init();
    if (i != ICM_OK) {
        ICM42688_Print("IMU Init Error: %d \r\n", i);
    }

    /* -----------------------------------------------------------
     * 3. 任务主循环
     * ----------------------------------------------------------- */
    for (;;) {
        /* 等待外部中断触发信号 */
        if (xTaskNotifyWait(0, SIGNAL_IMU_DATA_READY, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
            if (ulNotifiedValue & SIGNAL_IMU_DATA_READY) {
                
                /* 从 FIFO 中读取数据包 */
                ICM42688_ReadFIFO(FIFO_RxBuffer);
                ptr = FIFO_RxBuffer;
                P14 = 1;
                for (i = 0; i < icm_fifo_ths; i++) {
                    /* 解析数据包：计算物理加速度、角速度并提取原始 16bit Timestamp */
                    ICM42688_FIFO_Data_Handle(&IMU_Dat, ptr);

                    /* 检查 FIFO Header 的 Bit[3:2] 是否为 11 (FSYNC 标记) */
                    if((ptr[0] & 0x0C) == 0x0C) {
                        fsync_received = 1;
                        mcu_time = IMU_Dat.timestamp + fsync_time; 
                        
                        // 【修复 Bug 2】：防止第一次运行时产生巨大的 delta_mcu_time
                        if (last_mcu_time != 0) {
                            delta_mcu_time = mcu_time - last_mcu_time;
                        } else {
                            delta_mcu_time = 0; // 标记为 0，代表数据还不可用
                        }
                        last_mcu_time = mcu_time;
                        
                        // 覆盖 FSYNC 包错误的 sample_time
                        IMU_Dat.sensor.dat.sample_time = last_imu_sample_time;
                        
                    }else if(fsync_received) {
                        current_imu_time = IMU_Dat.timestamp;
                        
                        // 【修复 Bug 1】：确保 MCU 和 IMU 的历史时间都有效，才进行斜率计算
                        if(last_imu_time != 0 && delta_mcu_time > 0) {
                            delta_imu_time = current_imu_time - last_imu_time;
                            
                            // 必须确保分母不为 0
                            if (delta_imu_time > 0) {
                                float new_k = ((float)delta_mcu_time) / ((float)delta_imu_time);
                                imu_slope_k = new_k * 0.1f + imu_slope_k * 0.9f;
                            }
                        }
                        last_imu_time = current_imu_time;
                        fsync_received = 0;
                    }
                    IMU_Dat.sensor.dat.sample_time*=imu_slope_k;
                    last_imu_sample_time=IMU_Dat.sensor.dat.sample_time;
                    IMU_Filter_Data->sensor = IMU_Dat.sensor;
                    IMU_Filter_Update(IMU_Filter_Data); /* 内部会使用这个 dt 进行陀螺仪积分 */
                    ptr += 16; /* 移动到下一个包 */
                    
                    
                }

                /* -----------------------------------------------------------
                 * 4. FSYNC 信号发生器 (由任务模拟 200Hz 频率)
                 * ----------------------------------------------------------- */
                if(++tt >= 5) { /* 注意：需根据你的任务调用频率计算此处阈值 */
                    tt = 0;
                    taskENTER_CRITICAL();
                    P14 = 0;             /* 翻转引脚发出边沿信号给 FSYNC 引脚 */
                    fsync_time = Get_Tim9(); /* 记录 MCU 本地精密时间戳 */
                    taskEXIT_CRITICAL();
                }
            }
        }
    }
}

/* =======================================================
 * 对外接口
 * ======================================================= */
extern UserParams_t *UserParams;

void IMU_Init() {
    xTaskCreate(vIMUProcessTask, "IMU", 728, NULL, tskIDLE_PRIORITY + 4, NULL);

		//Debug_Register("ax", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->sensor.dat.ax));
    //Debug_Register("ay", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->sensor.dat.ay));
    //Debug_Register("az", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->sensor.dat.az));
   // Debug_Register("imu_k",DBG_FLOAT | STATIC_TYPE,&imu_slope_k);
    Debug_Register("delta", DBG_FLOAT | STATIC_TYPE, &(imu_slope_k));
    Debug_Register("pitch", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->pitch));
    Debug_Register("roll", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->roll));
    Debug_Register("yaw", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->yaw));
    Debug_Register("Kp", DBG_FLOAT | STATIC_TYPE, &(IMU_Filter_Data->Kp));
    Debug_Register("Ki", DBG_FLOAT | STATIC_TYPE, &(IMU_Filter_Data->Ki));
    Debug_Register("bias_gx", DBG_FLOAT | STATIC_TYPE, &(IMU_Filter_Data->bias_gx));
    Debug_Register("bias_gy", DBG_FLOAT | STATIC_TYPE, &(IMU_Filter_Data->bias_gy));
    Debug_Register("bias_gz", DBG_FLOAT | STATIC_TYPE, &(IMU_Filter_Data->bias_gz));
}

void IMU_Get_Angle(float* roll, float* pitch, float* yaw) {
    *roll = IMU_Filter_Data->roll;
    *pitch = IMU_Filter_Data->pitch;
    *yaw = IMU_Filter_Data->yaw;
}
