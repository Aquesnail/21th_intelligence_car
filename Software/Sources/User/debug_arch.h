#ifndef _DEBUG_ARCH_H
#define _DEBUG_ARCH_H

#include "stc32g144k246.h"  // 包含硬件寄存器定义
#include "dma_uart.h"
#include "FreeRTOS.h"
#include "task.h"


#include "vartype.h"

#define DataType_Msk    0x0F
#define FreqType_Msk    0x10

#define HIGH_FREQ_TYPE 0x10
#define LOW_FREQ_TYPE  0x00
void vDebugTask(void *pvParameters);
void Debug_Register(char* name, uint8_t type, void* addr);
void Debug_Log(const char* fmt,...);
void Debug_Init(void);
#endif