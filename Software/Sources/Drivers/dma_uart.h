
#ifndef _DMA_UART_H

#define _DMA_UART_H

#include "config.h"

#define UART_DMATxBufferLength 16   
#define UART_DMARxBufferLength 16     
#define UART_TxRingBufferLength 256
#define UART_RxRingBufferLength 256   

void UART5_Init(void);


uint16_t UART5_Write(char* str,uint16_t len);


#endif 