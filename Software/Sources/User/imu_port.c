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
uint16_t fsync_time=0;
uint8_t fsync_received=0;
uint8_t fsync_sent=0;
uint32_t imu_sample_time=0;
uint32_t imu_accurate_sample_time=0;

#define FSYNC_MCU_INTERVAL_US  10000.0f     // MCU 200Hz 发送频率对应的理想微秒数
#define SLOPE_ALPHA            0.02f       // 滤波系数：越小越平滑，抵御抖动能力越强
#define TICK_EXPECTED          4687        // 理想情况下 5ms 对应的 IMU tick (假设 32/30 比例)
#define TICK_ERROR_MARGIN      500         // 允许的偏差范围，超过则认为是噪声并跳过

static float imu_slope_k = 1.06667f;       // 初始斜率，默认值 32/30
static uint16_t last_fsync_imu_tick = 0;   // 上一次 FSYNC 时的 IMU 原始 tick
static uint32_t last_fsync_mcu_time = 0;   // 上一次 FSYNC 发生时的 MCU 本地时间戳
static uint8_t sync_initialized = 0;       // 标志位：是否完成了首次锁定

/* =======================================================
 * IMU 处理任务：基于 FSYNC 标记的斜率补偿与时间戳对齐
 * ======================================================= */
uint16_t delta_tick;
void vIMUProcessTask(void *pvParameters) {
    /* -----------------------------------------------------------
     * 1. 局部变量声明 (严格遵循 C89 标准，定义在函数开头)
     * ----------------------------------------------------------- */
    uint8_t xdata* ptr;
    uint16_t i;
    uint32_t ulNotifiedValue;
    uint8_t tt = 0;
    
    uint16_t current_imu_tick;
    static uint32_t imu_total_ticks = 0;   // 软件维护的 32位 IMU 总滴答数
	static uint16_t last_raw_ticks = 0;    // 上一次普通包的 16位原始值
    uint16_t ticks_since_sync;
    float k_instant;
    float dt_calculated; /* 计算出的单帧采样间隔 */
    
    /* 静态变量 */
    static uint16_t last_fsync_imu_tick = 0;   /* 上一次 FSYNC 包的 IMU 原始 tick */
    static uint32_t last_fsync_mcu_time = 0;   /* 上一次 FSYNC 的 MCU 参考时间 (us) */
    static uint32_t last_accurate_time = 0;    /* 上一帧的精确时间戳，用于求差得到 dt */
    static float imu_slope_k = 1.06667f;       /* 运行时的平滑斜率 */
    static uint8_t sync_initialized = 0;

    const float fsync_mcu_interval_us = 5000.0f; 
    const float alpha = 0.05f;

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
                    current_imu_tick = (uint16_t)((ptr[14] << 8) | ptr[15]); /* 获取当前包的绝对 IMU tick */

                    /* 检查 FIFO Header 的 Bit[3:2] 是否为 11 (FSYNC 标记) */
                    if((ptr[0] & 0x0C) == 0x0C) {
                        
                        /* A. FSYNC 同步帧逻辑：更新频率斜率 K */
                        if (sync_initialized) {
                            /* 利用无符号减法自动处理 16bit 溢出回绕 */
                            delta_tick = current_imu_tick - last_fsync_imu_tick;

                            /* 合理性校验：200Hz 下 delta_tick 理想值应在 4687 左右 (32k RC) */
                            if (delta_tick > 9000 && delta_tick < 11000) {
                                k_instant = fsync_mcu_interval_us / (float)delta_tick;
                                
                                /* 一阶低通滤波：平滑 IMU 与 MCU 的时钟比例系数 */
                                imu_slope_k = (imu_slope_k * (1.0f - alpha)) + (k_instant * alpha);
                            }
                        } else {
                            sync_initialized = 1;
                        }

                        /* 强制对齐基准：当前包的时间戳即为 MCU 记录的 fsync_time */
                        last_fsync_imu_tick = current_imu_tick;
                        last_fsync_mcu_time = fsync_time;
                        imu_accurate_sample_time = last_fsync_mcu_time;
                        fsync_received = 1;

                    } else {
                        /* B. 普通数据帧逻辑：基于上一个 FSYNC 基准点进行线性插值 */
                        if (sync_initialized) {
                            /* 计算距离上一个“对齐桩”过去了多少 tick */
                            ticks_since_sync = current_imu_tick - last_fsync_imu_tick;
                            
                            /* 计算公式：T_now = T_基准 + (Tick差值 * 动态斜率) */
                            imu_accurate_sample_time = last_fsync_mcu_time + (uint32_t)(ticks_since_sync * imu_slope_k);
                        } else {
                            /* 未完成首次锁定前，使用默认系数 */
                            imu_accurate_sample_time = (uint32_t)(current_imu_tick * 1.06667f);
                        }
                    }

                    /* 更新采样间隔 dt 并提交滤波解算 */
                    if (last_accurate_time != 0) {
                        /* 计算两次采样之间的物理时间差 (单位：秒) */
                        dt_calculated = (float)(imu_accurate_sample_time - last_accurate_time) / 1000000.0f;
                        
                        /* 异常保护：防止 dt 过大或为负 (例如时间戳回绕异常) */
                        if (dt_calculated <= 0.0f || dt_calculated > 0.01f) {
                            dt_calculated = 0.001f; /* 丢包或异常时 fallback 到 1kHz 理论值 */
                        }
                    } else {
                        dt_calculated = 0.001f; /* 第一帧默认值 */
                    }

                    /* 更新历史时间戳 */
                    last_accurate_time = imu_accurate_sample_time;

                    /* --- C. 提交滤波解算 --- */
                    IMU_Dat.sensor.dat.sample_time = dt_calculated; /* 这里现在是正确的 dt 了 */
                    
                    if (IMU_Filter_Data != NULL) {
                        IMU_Filter_Data->sensor = IMU_Dat.sensor;
                        IMU_Filter_Update(IMU_Filter_Data); /* 内部会使用这个 dt 进行陀螺仪积分 */
                    }
                    
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
    Debug_Register("imu_k",DBG_FLOAT | STATIC_TYPE,&imu_slope_k);
    Debug_Register("delta", DBG_UINT16 | HIGH_FREQ_TYPE, &(delta_tick));
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
