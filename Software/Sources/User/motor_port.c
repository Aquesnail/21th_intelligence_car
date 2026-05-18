#include "motor.h"
#include "spi3_user.h"
#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"

#include "debug_arch.h"
#include <stdarg.h>

#pragma warning disable = 138
#pragma warning disable = 151

#define SIGNAL_MOTOR_SPI_DMA_DONE  (1 << 0)

static uint8_t xdata MOTOR_DMA_TxBuf[32] = {0};

TaskHandle_t xMotorTaskHandle = NULL;

/* =======================================================
 * motor.h 要求的外部函数实现
 * ======================================================= */

/* SPI 单字节收发 (使用 SPI3) */
uint8_t MOTOR_SPI_TxRx(uint8_t dat) {
    uint8_t rx_dat;
    
    rx_dat = SPI3_WriteReadByte(dat);
    
    return rx_dat;
}

/* SPI DMA 批量读取 (使用 SPI3) */
void MOTOR_SPI_DmaRead(uint8_t *rx_buf, uint16_t len) {
    uint32_t ulNotifiedValue;
    memset(MOTOR_DMA_TxBuf, 0xFF, len);
    SPI3_DMA_WriteReadBytes_IT(MOTOR_DMA_TxBuf, rx_buf, len);
    if (xTaskNotifyWait(0, SIGNAL_MOTOR_SPI_DMA_DONE, &ulNotifiedValue, pdMS_TO_TICKS(10)) == pdTRUE) {
        return;
    }
}

/* 毫秒延时 */
void MOTOR_DelayMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* 获取系统 tick */
uint32_t MOTOR_GetTick(void) {
    return xTaskGetTickCount();
}

/* 调试打印 */
void MOTOR_Print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vDebug_Log(fmt, args);
    va_end(args);
}
