#ifndef _DMA_UART_H
#define _DMA_UART_H

#include "stc32g144k246_def.h"
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

// 外部接口
void UART5_Init(void);
uint16_t UART5_Write(char* dat, uint16_t len);
uint16_t UART5_Read(char* dat, uint16_t len);
uint16_t UART5_Available(void); // 查询接收缓冲区有多少数据

#endif