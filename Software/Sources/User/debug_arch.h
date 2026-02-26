#ifndef _DEBUG_ARCH_H
#define _DEBUG_ARCH_H

#include "stc32g144k246.h"  // 包含硬件寄存器定义
#include "dma_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "vartype.h"

#define DataType_Msk    0x0F
#define FreqType_Msk    0x10

#define HIGH_FREQ_TYPE 0x10
#define LOW_FREQ_TYPE  0x00

typedef void    (*debug_write_t)(uint8_t *pData,uint16_t len);     
typedef uint16_t    (*debug_read_t)(uint8_t *pData,uint16_t len);     

typedef struct{
    debug_read_t debug_read;
    debug_write_t debug_write;
}debug_port_t;
extern SemaphoreHandle_t xDebugReadySignal;
void vDebugTask(void *pvParameters);
void Debug_Register(char* name, uint8_t type, void* addr);
void Debug_Log(const char* fmt,...);
void Debug_LogLongString(char *buffer);
void Debug_Init(debug_port_t* debug_port_init_structure);
#endif