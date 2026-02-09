#ifndef _DEBUG_ARCH_H
#define _DEBUG_ARCH_H

#include "stc32g144k246.h"  // 包含硬件寄存器定义
#include "dma_uart.h"
#include "FreeRTOS.h"
#include "task.h"


#include "vartype.h"

void vDebugTask(void *pvParameters);
void Debug_Register(char* name, uint8_t type, void* addr);
void Debug_Log(char*str);
#endif