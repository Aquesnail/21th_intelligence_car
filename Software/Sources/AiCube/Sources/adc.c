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



////////////////////////////////////////
// ADC1初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void ADC1_Init(void)
{
    ADC1_SetClockDivider(0);            //设置ADC时钟
    ADC1_ResultRightAlign();            //设置ADC结果右对齐(12位结果)
    ADC1_DisablePWMTrig();              //禁止PWM同步触发ADC
    ADC1_ADC2_IndependMode();           //两组ADC为独立模式
    ADC1_DisableRepeatConv();           //关闭ADC自动重复转换功能

    ADC1_SetCSSetupCycles(0);           //设置ADC通道选择建立时间
    ADC1_SetCSHoldCycles(1);            //设置ADC通道选择保持时间
    ADC1_SetSampleDutyCycles(9);        //设置ADC通道采样时间

    ADC1_DisableETR();                  //禁止ADC外部触发功能

    ADC1_ActiveChannel(8);              //选择ADC通道
    ADC1_Enable();                      //使能ADC功能

    //<<AICUBE_USER_ADC1_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_ADC1_INITIAL_END>>
}

////////////////////////////////////////
// 获取ADC1转换结果函数
// 入口参数: ch (ADC通道选择)
// 函数返回: ADC转换结果
////////////////////////////////////////
uint16_t ADC1_Convert(uint8_t ch)
{
    uint16_t res;                       //定义保存ADC结果的变量

    ADC1_ActiveChannel(ch);             //选择ADC通道
    ADC1_Start();                       //开始ADC转换
    while (!ADC1_CheckFlag());          //等待ADC转换完成
    ADC1_ClearFlag();                   //清除ADC转换完成中断标志
    res = ADC1_ReadResult();            //读取ADC转换结果

    return res;                         //返回ADC结果
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


