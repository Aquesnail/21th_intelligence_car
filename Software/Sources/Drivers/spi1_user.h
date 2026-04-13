//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息
// 文件名称: spi1_user.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __SPI1_USER_H__
#define __SPI1_USER_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
//<<AICUBE_USER_DEFINE_END>>

typedef void (*DMA_SPI1_Callback_t)(BaseType_t *pxHigherPriorityTaskWoken);
void SPI1_Init(DMA_SPI1_Callback_t callback);
uint8_t SPI1_ReadByte(void);
void SPI1_WriteByte(uint8_t dat);
uint8_t SPI1_WriteReadByte(uint8_t dat);
//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明
void SPI1_DMA_ReadBytes_IT(uint8_t xdata*rx_buf, uint16_t len);
void SPI1_DMA_WriteReadBytes_IT(uint8_t xdata*tx_buf,uint8_t xdata*rx_buf, uint16_t len);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
