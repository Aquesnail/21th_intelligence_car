#include "adc_user.h"

// DMA 缓冲区定义 (必须位于 xdata 区域)
uint8_t * adc2_dma_buffer = NULL;

// DMA 完成标志
bit adc2_dma_done=0;

//========================================================================
// 函数: void ADC2_Init(void)
// 描述: ADC2 基础初始化，配置 P0.0~P0.3 为模拟输入 (ADCB0~3)
// 参数: none
// 返回: none
//========================================================================
void ADC2_Init(void)
{
    // 1. 配置 P0.0~P0.3 为高阻输入模式 (模拟输入)
    // P0M1=1, P0M0=0 表示高阻输入模式
//    P0M1 |= 0x0F;       // P0.0~P0.3 设置高阻
 //   P0M0 &= ~0x0F;      // P0.0~P0.3 清除推挽

    // 2. 禁止 P0 端口的数字输入，防止漏电流
  //  P0IE &= ~0x0F;      // 关闭 P0.0~P0.3 数字输入

    // 3. 配置 ADC2 时序
    // ADC2TIM: 通道选择时间、建立时间、保持时间
    // 0x3f 是默认值，可根据需要调整
    ADC2TIM = 0x3f;
    // 4. 配置 ADC2 速度和结果格式
    // bit5: 1=右对齐, 0=左对齐
    // bit3~0: 速度 0~15
    ADC2CFG = ADC2_RES_FMT | ADC2_SPEED;

    // 5. 打开 ADC2 电源，选择通道 0
    // bit7: 1=ADC 电源打开
    // bit3~0: 通道选择 (0~3 对应 P0.0~P0.3)
    ADC2_CONTR = 0x80;

    // 延时等待 ADC 电源稳定 (建议 1ms)
    delay_ms(1);
}


#define ADC2_STPCYC             0       //ADC2通道选择建立时间
#define ADC2_HLDCYC             1       //ADC2通道选择保持时间
#define ADC2_SMPCYC             14      //ADC2通道采样时间
#define ADC2_DMAITV             10000      //ADC2 DMA扫描间隔时间（系统时钟）
#define ADC2_DMAAMT             4   //ADC2 DMA扫描次数
#define ADC2_DMASIZE            (ADC2_DMAAMT * (8 * 2 + 4) * 4) //ADC2 DMA缓冲区大小
//========================================================================
// 函数: void ADC2_DMA_Init(void)
// 描述: ADC2 DMA 配置初始化
// 参数: none
// 返回: none
//========================================================================
void ADC2_DMA_Init(uint8_t* adc2_dma_buf)
{

    ADC2_SetClockDivider(3);           //设置ADC时钟
    ADC2_ResultRightAlign();            //设置ADC结果右对齐(12位结果)
    ADC2_DisablePWMTrig();              //禁止PWM同步触发ADC
    ADC2_DisableRepeatConv();           //关闭ADC自动重复转换功能

    ADC2_SetCSSetupCycles(ADC2_STPCYC); //设置ADC通道选择建立时间
    ADC2_SetCSHoldCycles(ADC2_HLDCYC);  //设置ADC通道选择保持时间
    ADC2_SetSampleDutyCycles(ADC2_SMPCYC); //设置ADC通道采样时间

    ADC2_DisableETR();                  //禁止ADC外部触发功能

    ADC2_ActiveChannel(0);              //选择ADC通道
    ADC2_Enable();                      //使能ADC功能

    DMA_ADC2_SetAmount(ADC2_DMAAMT - 1); //设置ADC DMA扫描次数
    //DMA_ADC2_SetAddress(pu8ADC2DMABuffer); //设置ADC DMA缓冲区地址
    DMA_ADC2_SetInterval(ADC2_DMAITV);  //设置ADC DMA扫描间隔时间（系统时钟）
    DMA_ADC2_SetChannels(0x000f);       //设置ADC DMA扫描通道
    DMA_ADC2_SetRepeat8Times();         //设置ADC DMA时每个通道重复转换次数
    DMA_ADC2_ClearFlag();               //清除ADC DMA中断标志
    DMA_ADC2_SetBusPriority(0);         //设置总线访问为最低优先级
    // // 1. 清除状态寄存器
    // DMA_ADC2_STA = 0x00;

    // // 2. 配置 DMA 中断使能
    // // bit7: 1=使能 DMA 完成中断
    DMA_ADC2_CFG |= 0x80;
    adc2_dma_buffer = adc2_dma_buf;
    // 3. 设置 DMA 接收地址 (高位和低位)
    DMA_ADC2_RXAH = (uint8_t)((uint16_t)adc2_dma_buffer >> 8);
    DMA_ADC2_RXAL = (uint8_t)adc2_dma_buffer;

    // // 4. 配置每个通道的 ADC 转换次数
    // // 公式: 2*转换次数 + 4，例如 16 次 = 2*6+4=0x10? 不对，看公式
    // // 官方例程: 4次 = 0x09 = 2*4+1? 实际上官方写的是 0x09 表示 4次
    // // 根据手册: n = (值 - 4) / 2，所以 16 次 = (2*16-4) = 28 = 0x1C
    // DMA_ADC2_CFG2 = 0b00001011;

    // // 5. 配置 ADC2 通道使能
    // // ADC2_0~ADC2_7 对应 P0.0~P0.7
    // // CHSW0: ADC7~ADC0 的使能位
     DMA_ADC2_CHSW0 |= 0x0F;      // 使能 ADC2_0~ADC2_3 (0x0F = 0000 1111)
    // // CHSW1: ADC15~ADC8 的使能位
     DMA_ADC2_CHSW1 |= 0x00;      // 不使能 8~15 通道

    DMA_ADC2_Enable();                  //使能ADC DMA功能
    // 6. 清除 DMA 完成标志
    adc2_dma_done = 0;
}

//========================================================================
// 函数: void ADC2_DMA_Start(void)
// 描述: 启动 ADC2 DMA 转换
// 参数: none
// 返回: none
//========================================================================
void ADC2_DMA_Start(void)
{
    // bit7: 1=使能 ADC2 DMA
    // bit6: 1=启动 ADC2 DMA
    DMA_ADC2_CR = 0xC0;
}

//========================================================================
// 函数: bit ADC2_DMA_IsDone(void)
// 描述: 检查 DMA 是否完成
// 参数: none
// 返回: 1=完成, 0=未完成
//========================================================================
bit ADC2_DMA_IsDone(void)
{
    return adc2_dma_done;
}

//========================================================================
// 函数: void ADC2_DMA_ClearFlag(void)
// 描述: 清除 DMA 完成标志
// 参数: none
// 返回: none
//========================================================================
void ADC2_DMA_ClearFlag(void)
{
    adc2_dma_done = 0;
}

//========================================================================
// 函数: uint16_t ADC2_GetRawValue(uint8_t ch)
// 描述: 获取指定通道的最后一次 ADC 采样值 (12位)
// 参数: ch: 通道号 0~3
// 返回: 12位 ADC 值
//========================================================================
uint16_t ADC2_GetRawValue(uint8_t ch)
{
    uint16_t adc_val;
    uint8_t idx;
    uint16_t offset;
    if (ch >= ADC2_CH_NUM) return 0;

    // 取最后一次采样值
    idx = ADC2_BUF_SIZE - 1;

    // 右对齐格式: 高8位在 RES，低8位在 RESL
    // DMA 缓冲区存储格式: 低字节在前，高字节在后
    offset = ch * ADC2_BUF_SIZE * 2 + idx * 2;
    adc_val = adc2_dma_buffer[offset];         // 低字节
    adc_val |= (uint16_t)adc2_dma_buffer[offset + 1] << 8;  // 高字节

    return adc_val & 0x0FFF;  // 只保留12位
}

//========================================================================
// 函数: uint16_t ADC2_GetAvgValue(uint8_t ch)
// 描述: 获取指定通道的平均 ADC 值 (12位)
// 参数: ch: 通道号 0~3
// 返回: 12位平均 ADC 值
//========================================================================
uint16_t ADC2_GetAvgValue(uint8_t ch)
{
    uint32_t sum = 0;
    uint16_t adc_val;
    uint8_t i;

    if (ch >= ADC2_CH_NUM) return 0;

    // 计算所有采样值的平均
    for (i = 0; i < ADC2_BUF_SIZE; i++)
    {
        uint16_t offset = ch * ADC2_BUF_SIZE * 2 + i * 2;
        adc_val = adc2_dma_buffer[offset];
        adc_val |= (uint16_t)adc2_dma_buffer[offset + 1] << 8;
        sum += (adc_val & 0x0FFF);
    }

    return (uint16_t)(sum / ADC2_BUF_SIZE);
}

//========================================================================
// 函数: void ADC2_DMA_ISR(void)
// 描述: ADC2 DMA 中断服务函数 (中断号 76)
// 参数: none
// 返回: none
// 注意: 在 main.c 中添加中断入口: void DMA_ADC2_IRQHandler(void) interrupt DMA_ADC2_VECTOR { ADC2_DMA_ISR(); }
//========================================================================
void DMA_ADC2_ISR_Handle(void* none) 
{
    // 清除 DMA 状态
    DMA_ADC2_STA = 0;

    // 设置完成标志
    adc2_dma_done = 1;
}
