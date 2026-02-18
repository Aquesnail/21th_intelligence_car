#ifndef _DMA_UART_H
#define _DMA_UART_H

#include "stc32g144k246_def.h"
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

// 缓冲区大小定义
#define UART_DMATxBufferLength 16   // DMA 物理发送缓冲区大小 (建议较小，如16-64)
#define UART_DMARxBufferLength 64   // DMA 物理接收缓冲区大小 (取决于最大帧长)
#define UART_TxRingBufferLength 1024 // 软件发送环形缓冲区
#define UART_RxRingBufferLength 1024 // 软件接收环形缓冲区

// 外部接口
void UART5_Init(void);
uint16_t UART5_Write(char* dat, uint16_t len);
uint16_t UART5_Read(char* dat, uint16_t len);
uint16_t UART5_Available(void); // 查询接收缓冲区有多少数据

#endif