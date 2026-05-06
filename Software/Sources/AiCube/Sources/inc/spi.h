//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: spi.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __SPI_H__
#define __SPI_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
//<<AICUBE_USER_DEFINE_END>>


#define SPI2_DMASIZE            16      //SPI2 DMA发送缓冲区大小
#define SPI2_DMAITV             0       //SPI2 DMA发送/接收字节间隔时间（系统时钟）


void SPI2_Init(void);
uint8_t SPI2_ReadByte(void);
void SPI2_WriteByte(uint8_t dat);

extern uint8_t xdata pu8SPI2DMATxBuffer[SPI2_DMASIZE];
extern uint8_t xdata pu8SPI2DMARxBuffer[SPI2_DMASIZE];


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
