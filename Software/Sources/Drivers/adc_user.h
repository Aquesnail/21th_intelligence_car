#ifndef __ADC_USER_H__
#define __ADC_USER_H__

#include "config.h"

// ADC2 配置参数
#define ADC2_SPEED      15      // 0~15, ADC转换时间 = (n+1)*32 个CPU时钟
#define ADC2_RES_FMT    (1<<5)  // 右对齐: ADC_RES: 0 0 0 0 D11 D10 D9 D8, ADC_RESL: D7 D6 D5 D4 D3 D2 D1 D0
#define ADC2_CH_NUM     8       // 通道数: P0.0~P0.7 对应 ADC2_0~ADC2_7
#define ADC2_BUF_SIZE   16      // 每个通道采样次数

// DMA 目标地址 (xdata 区域)
#define ADC2_DMA_ADDR   0x1000

// ADC2 初始化
void ADC2_Init(void);
void ADC2_DMA_Init(void);

// ADC2 启动 DMA 采样
void ADC2_DMA_Start(void);

// 检查 DMA 完成标志
bit ADC2_DMA_IsDone(void);
void ADC2_DMA_ClearFlag(void);

// 获取 ADC2 通道数据 (原始数据)
uint16_t ADC2_GetRawValue(uint8_t ch);
uint16_t ADC2_GetAvgValue(uint8_t ch);

// ADC2 DMA 中断服务函数 (需在 main.c 中包装成中断函数)
void ADC2_DMA_ISR(void);

#endif
