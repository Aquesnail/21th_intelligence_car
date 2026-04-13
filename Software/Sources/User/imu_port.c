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


#pragma warning disable = 138
#pragma warning disable = 151

#define FIFO_Packets 2

#define SIGNAL_IMU_DATA_READY  (1 << 0)  // 位 0：外部中断引脚触发
#define SIGNAL_SPI_DMA_DONE    (1 << 1)  // 位 1：DMA 传输完成 //任务通知比信号量快
/* =======================================================
 * 外部变量定义（供icm42688.c使用）
 * ======================================================= */
float   icm_acc_sensitivity;      /* 加速度灵敏度 */
float   icm_gyro_sensitivity;     /* 陀螺仪灵敏度 */
uint8_t icm_current_bank;         /* 当前寄存器bank */
uint16_t icm_fifo_ths;            /* FIFO阈值 */

SemaphoreHandle_t xDmaSemaphore;
TaskHandle_t   xImuTaskHandle;
static uint8_t xdata FIFO_RxBuffer[FIFO_Packets*16 +1];
static uint8_t xdata FIFO_TxBuffer[FIFO_Packets*16 +1];
IMU_Filter_t *IMU_Filter_Data;
static ICM42688_Pack16_t IMU_Dat;

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
}

/* 调试打印 */
void ICM42688_Print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Debug_Log(fmt, args);
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

/* =======================================================
 * IMU任务
 * ======================================================= */
float acc_t[3];
float gyro_t[3];
float temp_t;

void vIMUProcessTask(void *pvParameters) {
    uint8_t xdata* ptr;
    uint16_t i;
    uint32_t ulNotifiedValue;
    ptr = FIFO_RxBuffer;

    (void)pvParameters;

    xImuTaskHandle = xTaskGetCurrentTaskHandle();

    if (xDebugReadySignal != NULL) {
        xSemaphoreTake(xDebugReadySignal, portMAX_DELAY);
        xSemaphoreGive(xDebugReadySignal);
    }

    memset(FIFO_TxBuffer, 0xff, sizeof(FIFO_TxBuffer));

    vTaskDelay(pdMS_TO_TICKS(2));

    /* 初始化SPI */
    SPI2_Init(IMU_DMA_Read_CallBack);

    /* 设置FIFO阈值 */
    icm_fifo_ths = FIFO_Packets;

    /* 初始化ICM42688 */
    i = ICM42688_Init();
    if (i != ICM_OK) {
        Debug_Log("reg %d \r\n", i);
    }

    for (;;) {
        if (xTaskNotifyWait(0, SIGNAL_IMU_DATA_READY, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
            if (ulNotifiedValue & SIGNAL_IMU_DATA_READY) {
                ICM42688_ReadFIFO(FIFO_RxBuffer);
            

                ptr = FIFO_RxBuffer;
                for (i = 0; i < icm_fifo_ths; i++) {
                    ICM42688_FIFO_Data_Handle(&IMU_Dat, ptr);
                    /* Union共享：直接内存拷贝8个float */
                    IMU_Filter_Data->sensor = IMU_Dat.sensor;
                    IMU_Filter_Update(IMU_Filter_Data);
                    ptr += 16;
                }

            }
            

           // ICM42688_ReadData(acc_t, gyro_t, &temp_t);
        }
    }
}

/* =======================================================
 * 对外接口
 * ======================================================= */
extern UserParams_t *UserParams;

void IMU_Init() {
    xTaskCreate(vIMUProcessTask, "IMU", 728, NULL, tskIDLE_PRIORITY + 4, NULL);

    // Debug_Register("ax", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->sensor.dat.ax));
    // Debug_Register("ay", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->sensor.dat.ay));
    // Debug_Register("az", DBG_FLOAT | HIGH_FREQ_TYPE, &(IMU_Filter_Data->sensor.dat.az));
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
