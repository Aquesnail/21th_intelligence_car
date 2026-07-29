#ifndef _DMA_UART3_H
#define _DMA_UART3_H

#include "stc32g144k246_def.h"
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "dma_uart_ring.h"

extern dma_uart_manager_t uart3_mgr;

// 外部接口
void UART3_Init(void);

#endif