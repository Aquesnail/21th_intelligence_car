#ifndef _DEBUG_ARCH_H
#define _DEBUG_ARCH_H

#include "stc32g144k246.h"  // 包含硬件寄存器定义
#include "dma_uart.h"
#include "FreeRTOS.h"
#include "task.h"


/* --- 1. 扩充类型枚举 --- */
typedef enum {
    DBG_UINT8  = 0,
    DBG_INT8   = 1,
    DBG_UINT16 = 2,
    DBG_INT16  = 3,
    DBG_UINT32 = 4,
    DBG_INT32  = 5,
    DBG_FLOAT  = 6,
    DBG_STRING = 7
} VarType_t;

#define DataType_Msk    0x0F
#define FreqType_Msk    0x10

#define HIGH_FREQ_TYPE 0x10
#define LOW_FREQ_TYPE  0x00
void vDebugTask(void *pvParameters);
void Debug_Register(char* name, uint8_t type, void* addr);
void Debug_Log(char*str);
#endif