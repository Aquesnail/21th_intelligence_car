#ifndef __ADC_USER_H__
#define __ADC_USER_H__

#include "config.h"

// ADC2 配置参数
#define ADC2_SPEED      15      // 0~15, ADC转换时间 = (n+1)*32 个CPU时钟
#define ADC2_RES_FMT    (1<<5)  // 右对齐: ADC_RES: 0 0 0 0 D11 D10 D9 D8, ADC_RESL: D7 D6 D5 D4 D3 D2 D1 D0
#define ADC2_CH_NUM     4       // 通道数: P0.0~P0.3 对应 ADC2_0~ADC2_3 (ADCB0~3)
#define ADC2_BUF_SIZE   8      // 每个通道采样次数

// DMA 目标地址 (xdata 区域)
#define ADC2_DMA_ADDR   0x1000

#define ADC2_STPCYC             0       //ADC2通道选择建立时间
#define ADC2_HLDCYC             1       //ADC2通道选择保持时间
#define ADC2_SMPCYC             14      //ADC2通道采样时间
#define ADC2_DMAITV             0      //ADC2 DMA扫描间隔时间（系统时钟）
// ... 其他代码保持不变 ...

#define ADC2_DMAAMT             8       // ADC2 DMA 每个通道扫描次数(采样次数 n)

// 关键修正：单通道在 DMA 缓冲区占用的字节数 = (采样次数 n * 2) + 4 字节的尾部数据
#define ADC2_CH_BLOCK_SIZE      (ADC2_DMAAMT * 2 + 4) 

// 关键修正：ADC2 DMA 缓冲区总大小 = 通道数 * 单通道块大小
#define ADC2_DMASIZE            (64 * ADC2_CH_BLOCK_SIZE) 

// ADC2 初始化
void ADC2_Init(void);
void ADC2_DMA_Init(uint8_t* adc2_dma_buffer);

// ADC2 启动 DMA 采样
void ADC2_DMA_Start(void);

// 检查 DMA 完成标志
uint8_t ADC2_DMA_IsDone(void);
void ADC2_DMA_ClearFlag(void);

// 获取 ADC2 通道数据 (原始数据)
uint16_t ADC2_GetRawValue(uint8_t ch);
uint16_t ADC2_GetAvgValue(uint8_t ch);

// ADC2 DMA 中断服务函数 (需在 main.c 中包装成中断函数)
void ADC2_DMA_ISR(void);

//============================================================================
// ADC1 配置参数
//============================================================================
#define ADC1_SPEED      15      // 0~15, ADC转换时间 = (n+1)*32 个CPU时钟
#define ADC1_RES_FMT    (1<<5)  // 右对齐
#define ADC1_CH_NUM     4       // 通道数: ADC1, ADC4, ADC5, ADC6
#define ADC1_BUF_SIZE   8       // 每个通道采样次数

#define ADC1_STPCYC             0       // ADC1通道选择建立时间
#define ADC1_HLDCYC             1       // ADC1通道选择保持时间
#define ADC1_SMPCYC             14      // ADC1通道采样时间
#define ADC1_DMAITV             0       // ADC1 DMA扫描间隔时间（系统时钟）

#define ADC1_DMAAMT             8       // ADC1 DMA 每个通道扫描次数(采样次数 n)
#define ADC1_CH_BLOCK_SIZE      (ADC1_DMAAMT * 2 + 4)
#define ADC1_DMASIZE            (64 * ADC1_CH_BLOCK_SIZE)

// ADC1 初始化
void ADC1_Init(void);
void ADC1_DMA_Init(uint8_t* adc1_dma_buffer);

// ADC1 启动 DMA 采样
void ADC1_DMA_Start(void);

// 检查 DMA 完成标志
uint8_t ADC1_DMA_IsDone(void);
void ADC1_DMA_ClearFlag(void);

// 获取 ADC1 通道数据
uint16_t ADC1_GetAvgValue(uint8_t ch);

#endif
