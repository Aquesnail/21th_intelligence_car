//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: adc.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-03-30) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t xdata pu8ADC2DMABuffer[ADC2_DMASIZE]; //ADC2 DMA接收缓冲区数组

////////////////////////////////////////
// ADC2初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void ADC2_Init(void)
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
    DMA_ADC2_SetAddress(pu8ADC2DMABuffer); //设置ADC DMA缓冲区地址
    DMA_ADC2_SetInterval(ADC2_DMAITV);  //设置ADC DMA扫描间隔时间（系统时钟）
    DMA_ADC2_SetChannels(0x000f);       //设置ADC DMA扫描通道
    DMA_ADC2_SetRepeat8Times();         //设置ADC DMA时每个通道重复转换次数
    DMA_ADC2_ClearFlag();               //清除ADC DMA中断标志
    DMA_ADC2_SetBusPriority(0);         //设置总线访问为最低优先级
    DMA_ADC2_Enable();                  //使能ADC DMA功能
//  DMA_ADC2_Trigger();                 //触发ADC DMA

    //<<AICUBE_USER_ADC2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_ADC2_INITIAL_END>>
}

////////////////////////////////////////
// 获取ADC2转换结果函数
// 入口参数: ch (ADC通道选择)
// 函数返回: ADC转换结果
////////////////////////////////////////
uint16_t ADC2_Convert(uint8_t ch)
{
    uint16_t res;                       //定义保存ADC结果的变量

    ADC2_ActiveChannel(ch);             //选择ADC通道
    ADC2_Start();                       //开始ADC转换
    while (!ADC2_CheckFlag());          //等待ADC转换完成
    ADC2_ClearFlag();                   //清除ADC转换完成中断标志
    res = ADC2_ReadResult();            //读取ADC转换结果

    return res;                         //返回ADC结果
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


