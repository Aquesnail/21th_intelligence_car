//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: adc.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __ADC_H__
#define __ADC_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
//<<AICUBE_USER_DEFINE_END>>


#define ADC2_STPCYC             0       //ADC2通道选择建立时间
#define ADC2_HLDCYC             1       //ADC2通道选择保持时间
#define ADC2_SMPCYC             14      //ADC2通道采样时间
#define ADC2_DMAITV             10      //ADC2 DMA扫描间隔时间（系统时钟）
#define ADC2_DMAAMT             65536   //ADC2 DMA扫描次数
#define ADC2_DMASIZE            (ADC2_DMAAMT * (8 * 2 + 4) * 4) //ADC2 DMA缓冲区大小


void ADC2_Init(void);
uint16_t ADC2_Convert(uint8_t ch);

extern uint8_t xdata pu8ADC2DMABuffer[ADC2_DMASIZE];


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
