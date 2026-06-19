//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息
// 文件名称: usart_spi_dma.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __USART_SPI_DMA_H__
#define __USART_SPI_DMA_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
//<<AICUBE_USER_DEFINE_END>>

typedef void (*DMA_USART_SPI_Callback_t)(BaseType_t *pxHigherPriorityTaskWoken);
void USART1_SPI_Init(DMA_USART_SPI_Callback_t callback);
void USART1_SPI_DMA_WriteBytes_IT(uint8_t xdata *tx_buf, uint16_t len);
void USART1_SPI_DMA_ReadBytes_IT(uint8_t xdata *rx_buf, uint16_t len);
void USART1_SPI_DMA_WriteReadBytes_IT(uint8_t xdata *tx_buf, uint8_t xdata *rx_buf, uint16_t len);
//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
