#include "adc_user.h"

// DMA 缓冲区定义 (必须位于 xdata 区域)
uint8_t * adc2_dma_buffer = NULL;
uint8_t * adc1_dma_buffer = NULL;
// DMA 目标地址 (xdata 区域)
#define ADC2_DMA_ADDR   0x1000
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




// ... 其他代码保持不变 ...
//========================================================================
// 函数: void ADC2_DMA_Init(void)
// 描述: ADC2 DMA 配置初始化
// 参数: none
// 返回: none
//========================================================================
void ADC2_DMA_Init(uint8_t* adc2_dma_buf)
{

    ADC2_SetClockDivider(15);           //设置ADC时钟
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
  //  DMA_ADC2_SetAddress(pu8ADC2DMABuffer); //设置ADC DMA缓冲区地址
    DMA_ADC2_SetInterval(ADC2_DMAITV);  //设置ADC DMA扫描间隔时间（系统时钟）
    DMA_ADC2_SetChannels(0x000f);       //设置ADC DMA扫描通道
    DMA_ADC2_SetRepeat8Times();         //设置ADC DMA时每个通道重复转换次数
    DMA_ADC2_ClearFlag();               //清除ADC DMA中断标志
    DMA_ADC2_SetIntPriority(0);
    DMA_ADC2_SetBusPriority(0);         //设置总线访问为最低优先级
    DMA_ADC2_Enable();                  //使能ADC DMA功能
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

    // DMA_ADC2_SetChannels 已设置 CHSW0/1，无需重复

    // 清除 DMA 完成标志
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
    if (DMA_ADC2_CR & 0x40) return;
    DMA_ADC2_CR = 0xC0;
}

//========================================================================
// 函数: bit ADC2_DMA_IsDone(void)
// 描述: 检查 DMA 是否完成
// 参数: none
// 返回: 1=完成, 0=未完成
//========================================================================
uint8_t ADC2_DMA_IsDone(void)
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
// 函数: uint16_t ADC2_GetAvgValue(uint8_t ch)
// 描述: 获取指定通道的平均 ADC 值 (12位)
// 参数: ch: 通道号 0~3
// 返回: 12位平均 ADC 值
//========================================================================
//========================================================================
// 函数: uint16_t ADC2_GetAvgValue(uint8_t ch)
// 描述: 获取指定通道的硬件平均 ADC 值 (12位)
//========================================================================
uint16_t ADC2_GetAvgValue(uint8_t ch)
{
    uint16_t offset;
    
    uint16_t high_byte, low_byte;

    if (ch >= ADC2_CH_NUM) return 0;

    // 1. 计算当前通道在数组中的基础偏移量 (通道号 * 单通道块大小)
    offset = ch * ADC2_CH_BLOCK_SIZE + (ADC2_DMAAMT * 2);

    // 2. 定位到硬件计算平均值的基础位置 (偏移为 2n)
    // n 是采样次数 ADC2_DMAAMT
   

    // 3. 读取平均值的高低字节 (手册规定位置在 2n+2 和 2n+3)
    high_byte = adc2_dma_buffer[offset + 2];
    low_byte  = adc2_dma_buffer[offset + 3];

    // 4. 拼接并返回 (屏蔽高4位，保留12位有效数据)
    return ((high_byte << 8) | low_byte) & 0x0FFF;
}
//========================================================================
// 函数: void ADC2_DMA_ISR(void)
// 描述: ADC2 DMA 中断服务函数 (中断号 76)
// 参数: none
// 返回: none
// 注意: 在 main.c 中添加中断入口: void DMA_ADC2_IRQHandler(void) interrupt DMA_ADC2_VECTOR { ADC2_DMA_ISR(); }
//========================================================================
void DMA_ADC2_ISR_Handle() interrupt DMA_ADC2_VECTOR
{
    // 清除 DMA 状态
    DMA_ADC2_STA = 0;

    // 设置完成标志
    adc2_dma_done = 1;

    return;
}

//============================================================================
// ADC1 配置代码 (参照 ADC2)
//============================================================================

// DMA 缓冲区定义

// DMA 完成标志
bit adc1_dma_done = 0;

//========================================================================
// 函数: void ADC1_Init(void)
// 描述: ADC1 基础初始化
// 参数: none
// 返回: none
//========================================================================
void ADC1_Init(void)
{
    // 1. 配置 ADC1 时序
    ADCTIM = 0x3f;

    // 2. 配置 ADC1 速度和结果格式
    ADCCFG = ADC1_RES_FMT | ADC1_SPEED;

    // 3. 打开 ADC1 电源
    ADC_CONTR = 0x80;

    // 延时等待 ADC 电源稳定
    delay_ms(1);
}

//========================================================================
// 函数: void ADC1_DMA_Init(void)
// 描述: ADC1 DMA 配置初始化
// 参数: adc1_dma_buf: DMA 缓冲区指针
// 返回: none
//========================================================================
void ADC1_DMA_Init(uint8_t* adc1_dma_buf)
{
    ADC1_SetClockDivider(15);
    ADC1_ResultRightAlign();
    ADC1_DisablePWMTrig();
    ADC1_DisableRepeatConv();

    ADC1_SetCSSetupCycles(ADC1_STPCYC);
    ADC1_SetCSHoldCycles(ADC1_HLDCYC);
    ADC1_SetSampleDutyCycles(ADC1_SMPCYC);

    ADC1_DisableETR();

    ADC1_ActiveChannel(0);
    ADC1_Enable();

    DMA_ADC1_SetAmount(ADC1_DMAAMT - 1);
    DMA_ADC1_SetInterval(ADC1_DMAITV);
    DMA_ADC1_SetChannels(0x0072);  // ADC1, ADC4, ADC5, ADC6 (bit1,4,5,6)
    DMA_ADC1_SetRepeat8Times();
    DMA_ADC1_ClearFlag();
    DMA_ADC1_SetIntPriority(0);
    DMA_ADC1_SetBusPriority(0);
    DMA_ADC1_Enable();

    // 使能 DMA 完成中断
    DMA_ADC_CFG |= 0x80;
    adc1_dma_buffer = adc1_dma_buf;

    // 设置 DMA 接收地址
    DMA_ADC_RXAH = (uint8_t)((uint16_t)adc1_dma_buffer >> 8);
    DMA_ADC_RXAL = (uint8_t)adc1_dma_buffer;

    // DMA_ADC1_SetChannels 已设置 CHSW0/1，无需重复

    // 清除 DMA 完成标志
    adc1_dma_done = 0;
}

//========================================================================
// 函数: void ADC1_DMA_Start(void)
// 描述: 启动 ADC1 DMA 转换
// 参数: none
// 返回: none
//========================================================================
void ADC1_DMA_Start(void)
{
    if (DMA_ADC_CR & 0x40) return;
    DMA_ADC_CR = 0xC0;
}

//========================================================================
// 函数: uint8_t ADC1_DMA_IsDone(void)
// 描述: 检查 DMA 是否完成
// 参数: none
// 返回: 1=完成, 0=未完成
//========================================================================
uint8_t ADC1_DMA_IsDone(void)
{
    return adc1_dma_done;
}

//========================================================================
// 函数: void ADC1_DMA_ClearFlag(void)
// 描述: 清除 DMA 完成标志
// 参数: none
// 返回: none
//========================================================================
void ADC1_DMA_ClearFlag(void)
{
    adc1_dma_done = 0;
}

//========================================================================
// 函数: uint16_t ADC1_GetAvgValue(uint8_t ch)
// 描述: 获取指定通道的硬件平均 ADC 值 (12位)
// 参数: ch: 通道号 0~3
// 返回: 12位平均 ADC 值
//========================================================================
uint16_t ADC1_GetAvgValue(uint8_t ch)
{
    uint16_t offset;
    uint16_t high_byte, low_byte;

    if (ch >= ADC1_CH_NUM) return 0;

    // 计算当前通道在数组中的基础偏移量
    offset = ch * ADC1_CH_BLOCK_SIZE + (ADC1_DMAAMT * 2);

    // 读取平均值的高低字节
    high_byte = adc1_dma_buffer[offset + 2];
    low_byte  = adc1_dma_buffer[offset + 3];

    // 拼接并返回 (屏蔽高4位，保留12位有效数据)
    return ((high_byte << 8) | low_byte) & 0x0FFF;
}

//========================================================================
// 函数: void DMA_ADC1_ISR_Handle(void)
// 描述: ADC1 DMA 中断服务函数 (中断号 48)
// 参数: none
// 返回: none
//========================================================================
void DMA_ADC1_ISR_Handle() interrupt DMA_ADC1_VECTOR
{
    // 清除 DMA 状态
    DMA_ADC_STA = 0;

    // 设置完成标志
    adc1_dma_done = 1;

    return;
}
