//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: pwm.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-01-17) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
#include "pwm.h" 
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t xdata pu8PWMEDMATxBuffer[12];   //PWMEDMA发送缓冲区数组

////////////////////////////////////////
// PWMA初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PWMA_Init(void)
{
    PWMA_C3SwitchP6465();               //设置PWMA通道3数据端口: PWMA3P (P6.4), PWMA3N (P6.5)
    PWMA_C4SwitchP6667();               //设置PWMA通道4数据端口: PWMA4P (P6.6), PWMA4N (P6.7)

    HSPWMA_Disable();                   //关闭高速模式，使用同步方式进行初始化
    PWMA_InternalClockMode();           //选择内部时钟模式
    PWMA_SetClockDivider(1199);        //设置16位预分频
    PWMA_SetReload16(999);              //设置16位重载值
    PWMA_BufferARR();                   //使能重载值寄存器预装载功能
    PWMA_SetCounter(0);                 //初始化计数值

    PWMA_CC3PDisable();                 //关闭通道
    PWMA_CC3Output();                   //使能通道输出功能
    PWMA_CC3PEnable();                  //打开通道
    PWM_UpdateDuty(PWMA_CH3, 0);        //设置通道的PWM占空比
    PWMA_OC3REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWMA_BufferCCR3();                  //使能CCRn预装载功能
    PWMA_CC3PNonInverted();             //设置正极通道输出高电平有效
    PWMA_EnablePWM3POutput();           //使能正极通道输出
    PWMA_DisablePWM3NOutput();          //关闭负极通道输出

    PWMA_CC4PDisable();                 //关闭通道
    PWMA_CC4Output();                   //使能通道输出功能
    PWMA_CC4PEnable();                  //打开通道
    PWM_UpdateDuty(PWMA_CH4, 0);        //设置通道的PWM占空比
    PWMA_OC4REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWMA_BufferCCR4();                  //使能CCRn预装载功能
    PWMA_CC4PNonInverted();             //设置正极通道输出高电平有效
    PWMA_EnablePWM4POutput();           //使能正极通道输出
    PWMA_DisablePWM4NOutput();          //关闭负极通道输出

    PWMA_EnableMainOutput();            //使能PWM主输出

    PWMA_Run();                         //PWMA开始运行

    //<<AICUBE_USER_PWM0_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PWM0_INITIAL_END>>
}

////////////////////////////////////////
// PWME初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PWME_Init(void)
{
    PWME_C5SwitchP95();                 //设置PWME通道5数据端口: PWME5P (P9.5)

    HSPWME_Disable();                   //关闭高速模式，使用同步方式进行初始化
    PWME_InternalClockMode();           //选择内部时钟模式
    PWME_SetClockDivider(1);            //设置16位预分频
    PWME_SetReload16(74);               //设置16位重载值
    PWME_BufferARR();                   //使能重载值寄存器预装载功能
    PWME_SetCounter(0);                 //初始化计数值

    PWME_CC5PEnable();                  //打开通道
    PWM_UpdateDuty(PWME_CH5, 4);        //设置通道的PWM占空比
    PWME_OC5REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWME_BufferCCR5();                  //使能CCRn预装载功能
    PWME_CC5PNonInverted();             //设置正极通道输出高电平有效
    PWME_EnablePWM5POutput();           //使能正极通道输出

    PWME_EnableMainOutput();            //使能PWM主输出

    PWME_UpdateIntAsDMAEvent();         //设置PWME更新中断作为DMA事件
    PWME_SetDMABaseAddress(22);         //设置PWME DMA的寄存器基地址
    PWME_SetDMABurstCount(0);           //设置PWME DMA突发传输次数
    PWME_SetDMABurst4Bytes();           //设置PWME DMA突发传输字节数
    PWME_DMANeverSkipSpace();           //设置PWME DMA时不跳过保留寄存器（会对保留字节进行无效读写）
    PWME_SetDMAOutput();                //设置PWME DMA方向为输出（DMA输出PWM波形）

    DMA_PWME_SetTxAmount(11);           //设置PWME发送DMA总字节数
    DMA_PWME_SetTxAddress(pu8PWMEDMATxBuffer); //设置PWME发送DMA缓冲区地址
    DMA_PWME_SetInterval(0);            //设置PWME发送DMA字节间隔时间（系统时钟）
    DMA_PWME_ClearTxFlag();             //清除PWME发送DMA中断标志
    DMA_PWME_SetTxBusPriority(0);       //设置总线访问为最低优先级
    DMA_PWME_EnableTx();                //使能PWME发送DMA功能
    PWME_EnableDMA();                   //设置PWME DMA方向为输出（DMA输出PWM波形）

    PWME_Run();                         //PWME开始运行
//  DMA_PWME_TriggerTx();               //触发PWME发送DMA

    //<<AICUBE_USER_PWM4_INITIAL_BEGIN>>
    PWME_EnableUpdate(); // 确保PWM计数器溢出时能产生事件
    //<<AICUBE_USER_PWM4_INITIAL_END>>
}

////////////////////////////////////////
// 设置PWM通道输出占空比
// 入口参数: PWMx: (目标PWM组和通道索引)
//           nCompare: (PWM占空比值)
// 函数返回: 无
////////////////////////////////////////
void PWM_UpdateDuty(uint8_t PWMx, uint16_t nCompare)
{
    switch (PWMx)
    {
    case PWMA_CH1:
        PWMA_SetCCR1Value(nCompare);    //设置通道比较值
        break;
    case PWMA_CH2:
        PWMA_SetCCR2Value(nCompare);    //设置通道比较值
        break;
    case PWMA_CH3:
        PWMA_SetCCR3Value(nCompare);    //设置通道比较值
        break;
    case PWMA_CH4:
        PWMA_SetCCR4Value(nCompare);    //设置通道比较值
        break;
    case PWMA_CH5:
        PWMA_SetCCR5Value(nCompare);    //设置通道比较值
        break;
    case PWMA_CH6:
        PWMA_SetCCR6Value(nCompare);    //设置通道比较值
        break;
    case PWMB_CH5:
        PWMB_SetCCR5Value(nCompare);    //设置通道比较值
        break;
    case PWMB_CH6:
        PWMB_SetCCR6Value(nCompare);    //设置通道比较值
        break;
    case PWMB_CH7:
        PWMB_SetCCR7Value(nCompare);    //设置通道比较值
        break;
    case PWMB_CH8:
        PWMB_SetCCR8Value(nCompare);    //设置通道比较值
        break;
    case PWMC_CH1:
        PWMC_SetCCR1Value(nCompare);    //设置通道比较值
        break;
    case PWMC_CH2:
        PWMC_SetCCR2Value(nCompare);    //设置通道比较值
        break;
    case PWMC_CH3:
        PWMC_SetCCR3Value(nCompare);    //设置通道比较值
        break;
    case PWMC_CH4:
        PWMC_SetCCR4Value(nCompare);    //设置通道比较值
        break;
    case PWMC_CH5:
        PWMC_SetCCR5Value(nCompare);    //设置通道比较值
        break;
    case PWMC_CH6:
        PWMC_SetCCR6Value(nCompare);    //设置通道比较值
        break;
    case PWMD_CH5:
        PWMD_SetCCR5Value(nCompare);    //设置通道比较值
        break;
    case PWMD_CH6:
        PWMD_SetCCR6Value(nCompare);    //设置通道比较值
        break;
    case PWMD_CH7:
        PWMD_SetCCR7Value(nCompare);    //设置通道比较值
        break;
    case PWMD_CH8:
        PWMD_SetCCR8Value(nCompare);    //设置通道比较值
        break;
    case PWME_CH1:
        PWME_SetCCR1Value(nCompare);    //设置通道比较值
        break;
    case PWME_CH2:
        PWME_SetCCR2Value(nCompare);    //设置通道比较值
        break;
    case PWME_CH3:
        PWME_SetCCR3Value(nCompare);    //设置通道比较值
        break;
    case PWME_CH4:
        PWME_SetCCR4Value(nCompare);    //设置通道比较值
        break;
    case PWME_CH5:
        PWME_SetCCR5Value(nCompare);    //设置通道比较值
        break;
    case PWME_CH6:
        PWME_SetCCR6Value(nCompare);    //设置通道比较值
        break;
    case PWMF_CH5:
        PWMF_SetCCR5Value(nCompare);    //设置通道比较值
        break;
    case PWMF_CH6:
        PWMF_SetCCR6Value(nCompare);    //设置通道比较值
        break;
    case PWMF_CH7:
        PWMF_SetCCR7Value(nCompare);    //设置通道比较值
        break;
    case PWMF_CH8:
        PWMF_SetCCR8Value(nCompare);    //设置通道比较值
        break;
    }
}

////////////////////////////////////////
// 读取PWM通道捕获值
// 入口参数: PWMx: (目标PWM组和通道索引)
// 函数返回: 捕获值
////////////////////////////////////////
uint16_t PWM_ReadCapture(uint8_t PWMx)
{
    uint16_t cap;

    switch (PWMx)
    {
    case PWMA_CH1:  cap = PWMA_ReadCCR1Value(); break;
    case PWMA_CH2:  cap = PWMA_ReadCCR2Value(); break;
    case PWMA_CH3:  cap = PWMA_ReadCCR3Value(); break;
    case PWMA_CH4:  cap = PWMA_ReadCCR4Value(); break;
    case PWMA_CH5:  cap = PWMA_ReadCCR5Value(); break;
    case PWMA_CH6:  cap = PWMA_ReadCCR6Value(); break;
    case PWMB_CH5:  cap = PWMB_ReadCCR5Value(); break;
    case PWMB_CH6:  cap = PWMB_ReadCCR6Value(); break;
    case PWMB_CH7:  cap = PWMB_ReadCCR7Value(); break;
    case PWMB_CH8:  cap = PWMB_ReadCCR8Value(); break;
    case PWMC_CH1:  cap = PWMC_ReadCCR1Value(); break;
    case PWMC_CH2:  cap = PWMC_ReadCCR2Value(); break;
    case PWMC_CH3:  cap = PWMC_ReadCCR3Value(); break;
    case PWMC_CH4:  cap = PWMC_ReadCCR4Value(); break;
    case PWMC_CH5:  cap = PWMC_ReadCCR5Value(); break;
    case PWMC_CH6:  cap = PWMC_ReadCCR6Value(); break;
    case PWMD_CH5:  cap = PWMD_ReadCCR5Value(); break;
    case PWMD_CH6:  cap = PWMD_ReadCCR6Value(); break;
    case PWMD_CH7:  cap = PWMD_ReadCCR7Value(); break;
    case PWMD_CH8:  cap = PWMD_ReadCCR8Value(); break;
    case PWME_CH1:  cap = PWME_ReadCCR1Value(); break;
    case PWME_CH2:  cap = PWME_ReadCCR2Value(); break;
    case PWME_CH3:  cap = PWME_ReadCCR3Value(); break;
    case PWME_CH4:  cap = PWME_ReadCCR4Value(); break;
    case PWME_CH5:  cap = PWME_ReadCCR5Value(); break;
    case PWME_CH6:  cap = PWME_ReadCCR6Value(); break;
    case PWMF_CH5:  cap = PWMF_ReadCCR5Value(); break;
    case PWMF_CH6:  cap = PWMF_ReadCCR6Value(); break;
    case PWMF_CH7:  cap = PWMF_ReadCCR7Value(); break;
    case PWMF_CH8:  cap = PWMF_ReadCCR8Value(); break;
    default: cap = 0;
    }

    return cap;
}

////////////////////////////////////////
// 设置高速模式PWM通道输出占空比
// 入口参数: PWMx: (目标PWM组和通道索引)
//           nCompare: (PWM占空比值)
// 函数返回: 无
////////////////////////////////////////
void HSPWM_UpdateDuty(uint8_t PWMx, uint16_t nCompare)
{
    switch (PWMx)
    {
    case PWMA_CH1:
        HSPWMA_SetCCR1Value(nCompare);  //设置通道比较值
        break;
    case PWMA_CH2:
        HSPWMA_SetCCR2Value(nCompare);  //设置通道比较值
        break;
    case PWMA_CH3:
        HSPWMA_SetCCR3Value(nCompare);  //设置通道比较值
        break;
    case PWMA_CH4:
        HSPWMA_SetCCR4Value(nCompare);  //设置通道比较值
        break;
    case PWMA_CH5:
        HSPWMA_SetCCR5Value(nCompare);  //设置通道比较值
        break;
    case PWMA_CH6:
        HSPWMA_SetCCR6Value(nCompare);  //设置通道比较值
        break;
    case PWMB_CH5:
        HSPWMB_SetCCR5Value(nCompare);  //设置通道比较值
        break;
    case PWMB_CH6:
        HSPWMB_SetCCR6Value(nCompare);  //设置通道比较值
        break;
    case PWMB_CH7:
        HSPWMB_SetCCR7Value(nCompare);  //设置通道比较值
        break;
    case PWMB_CH8:
        HSPWMB_SetCCR8Value(nCompare);  //设置通道比较值
        break;
    case PWMC_CH1:
        HSPWMC_SetCCR1Value(nCompare);  //设置通道比较值
        break;
    case PWMC_CH2:
        HSPWMC_SetCCR2Value(nCompare);  //设置通道比较值
        break;
    case PWMC_CH3:
        HSPWMC_SetCCR3Value(nCompare);  //设置通道比较值
        break;
    case PWMC_CH4:
        HSPWMC_SetCCR4Value(nCompare);  //设置通道比较值
        break;
    case PWMC_CH5:
        HSPWMC_SetCCR5Value(nCompare);  //设置通道比较值
        break;
    case PWMC_CH6:
        HSPWMC_SetCCR6Value(nCompare);  //设置通道比较值
        break;
    case PWMD_CH5:
        HSPWMD_SetCCR5Value(nCompare);  //设置通道比较值
        break;
    case PWMD_CH6:
        HSPWMD_SetCCR6Value(nCompare);  //设置通道比较值
        break;
    case PWMD_CH7:
        HSPWMD_SetCCR7Value(nCompare);  //设置通道比较值
        break;
    case PWMD_CH8:
        HSPWMD_SetCCR8Value(nCompare);  //设置通道比较值
        break;
    case PWME_CH1:
        HSPWME_SetCCR1Value(nCompare);  //设置通道比较值
        break;
    case PWME_CH2:
        HSPWME_SetCCR2Value(nCompare);  //设置通道比较值
        break;
    case PWME_CH3:
        HSPWME_SetCCR3Value(nCompare);  //设置通道比较值
        break;
    case PWME_CH4:
        HSPWME_SetCCR4Value(nCompare);  //设置通道比较值
        break;
    case PWME_CH5:
        HSPWME_SetCCR5Value(nCompare);  //设置通道比较值
        break;
    case PWME_CH6:
        HSPWME_SetCCR6Value(nCompare);  //设置通道比较值
        break;
    case PWMF_CH5:
        HSPWMF_SetCCR5Value(nCompare);  //设置通道比较值
        break;
    case PWMF_CH6:
        HSPWMF_SetCCR6Value(nCompare);  //设置通道比较值
        break;
    case PWMF_CH7:
        HSPWMF_SetCCR7Value(nCompare);  //设置通道比较值
        break;
    case PWMF_CH8:
        HSPWMF_SetCCR8Value(nCompare);  //设置通道比较值
        break;
    }
}

////////////////////////////////////////
// 读取高速模式PWM通道捕获值
// 入口参数: PWMx: (目标PWM组和通道索引)
// 函数返回: 捕获值
////////////////////////////////////////
uint16_t HSPWM_ReadCapture(uint8_t PWMx)
{
    uint16_t cap;

    switch (PWMx)
    {
    case PWMA_CH1:  cap = HSPWMA_ReadCCR1Value();   break;
    case PWMA_CH2:  cap = HSPWMA_ReadCCR2Value();   break;
    case PWMA_CH3:  cap = HSPWMA_ReadCCR3Value();   break;
    case PWMA_CH4:  cap = HSPWMA_ReadCCR4Value();   break;
    case PWMA_CH5:  cap = HSPWMA_ReadCCR5Value();   break;
    case PWMA_CH6:  cap = HSPWMA_ReadCCR6Value();   break;
    case PWMB_CH5:  cap = HSPWMB_ReadCCR5Value();   break;
    case PWMB_CH6:  cap = HSPWMB_ReadCCR6Value();   break;
    case PWMB_CH7:  cap = HSPWMB_ReadCCR7Value();   break;
    case PWMB_CH8:  cap = HSPWMB_ReadCCR8Value();   break;
    case PWMC_CH1:  cap = HSPWMC_ReadCCR1Value();   break;
    case PWMC_CH2:  cap = HSPWMC_ReadCCR2Value();   break;
    case PWMC_CH3:  cap = HSPWMC_ReadCCR3Value();   break;
    case PWMC_CH4:  cap = HSPWMC_ReadCCR4Value();   break;
    case PWMC_CH5:  cap = HSPWMC_ReadCCR5Value();   break;
    case PWMC_CH6:  cap = HSPWMC_ReadCCR6Value();   break;
    case PWMD_CH5:  cap = HSPWMD_ReadCCR5Value();   break;
    case PWMD_CH6:  cap = HSPWMD_ReadCCR6Value();   break;
    case PWMD_CH7:  cap = HSPWMD_ReadCCR7Value();   break;
    case PWMD_CH8:  cap = HSPWMD_ReadCCR8Value();   break;
    case PWME_CH1:  cap = HSPWME_ReadCCR1Value();   break;
    case PWME_CH2:  cap = HSPWME_ReadCCR2Value();   break;
    case PWME_CH3:  cap = HSPWME_ReadCCR3Value();   break;
    case PWME_CH4:  cap = HSPWME_ReadCCR4Value();   break;
    case PWME_CH5:  cap = HSPWME_ReadCCR5Value();   break;
    case PWME_CH6:  cap = HSPWME_ReadCCR6Value();   break;
    case PWMF_CH5:  cap = HSPWMF_ReadCCR5Value();   break;
    case PWMF_CH6:  cap = HSPWMF_ReadCCR6Value();   break;
    case PWMF_CH7:  cap = HSPWMF_ReadCCR7Value();   break;
    case PWMF_CH8:  cap = HSPWMF_ReadCCR8Value();   break;
    default: cap = 0;
    }

    return cap;
}

////////////////////////////////////////
// 异步方式读取PWMA特殊功能寄存器
// 入口参数: addr: (PWMA特殊功能寄存器的低7位)
// 函数返回: 寄存器的值
////////////////////////////////////////;
uint8_t HSPWMA_ReadReg(uint8_t addr)
{
    uint8_t dat;

    while (HSPWMA_CheckBusy());         //等待前一个异步读写完成
    HSPWMA_AsyncRead(addr, dat);        //触发异步方式读取寄存器

    return dat;
}

////////////////////////////////////////
// 异步方式写PWMA特殊功能寄存器
// 入口参数: addr: (PWMA特殊功能寄存器的低7位)
//           dat: (待写入的数据)
// 函数返回: 无
////////////////////////////////////////;
void HSPWMA_WriteReg(uint8_t addr, uint8_t dat)
{
    while (HSPWMA_CheckBusy());         //等待前一个异步读写完成
    HSPWMA_AsyncWrite(addr, dat);       //触发异步方式写寄存器
}

////////////////////////////////////////
// 异步方式读取PWMB特殊功能寄存器
// 入口参数: addr: (PWMB特殊功能寄存器的低7位)
// 函数返回: 寄存器的值
////////////////////////////////////////;
uint8_t HSPWMB_ReadReg(uint8_t addr)
{
    uint8_t dat;

    while (HSPWMB_CheckBusy());         //等待前一个异步读写完成
    HSPWMB_AsyncRead(addr, dat);        //触发异步方式读取寄存器

    return dat;
}

////////////////////////////////////////
// 异步方式写PWMB特殊功能寄存器
// 入口参数: addr: (PWMB特殊功能寄存器的低7位)
//           dat: (待写入的数据)
// 函数返回: 无
////////////////////////////////////////;
void HSPWMB_WriteReg(uint8_t addr, uint8_t dat)
{
    while (HSPWMB_CheckBusy());         //等待前一个异步读写完成
    HSPWMB_AsyncWrite(addr, dat);       //触发异步方式写寄存器
}

////////////////////////////////////////
// 异步方式读取PWMC特殊功能寄存器
// 入口参数: addr: (PWMC特殊功能寄存器的低7位)
// 函数返回: 寄存器的值
////////////////////////////////////////;
uint8_t HSPWMC_ReadReg(uint8_t addr)
{
    uint8_t dat;

    while (HSPWMC_CheckBusy());         //等待前一个异步读写完成
    HSPWMC_AsyncRead(addr, dat);        //触发异步方式读取寄存器

    return dat;
}

////////////////////////////////////////
// 异步方式写PWMC特殊功能寄存器
// 入口参数: addr: (PWMC特殊功能寄存器的低7位)
//           dat: (待写入的数据)
// 函数返回: 无
////////////////////////////////////////;
void HSPWMC_WriteReg(uint8_t addr, uint8_t dat)
{
    while (HSPWMC_CheckBusy());         //等待前一个异步读写完成
    HSPWMC_AsyncWrite(addr, dat);       //触发异步方式写寄存器
}

////////////////////////////////////////
// 异步方式读取PWMD特殊功能寄存器
// 入口参数: addr: (PWMD特殊功能寄存器的低7位)
// 函数返回: 寄存器的值
////////////////////////////////////////;
uint8_t HSPWMD_ReadReg(uint8_t addr)
{
    uint8_t dat;

    while (HSPWMD_CheckBusy());         //等待前一个异步读写完成
    HSPWMD_AsyncRead(addr, dat);        //触发异步方式读取寄存器

    return dat;
}

////////////////////////////////////////
// 异步方式写PWMD特殊功能寄存器
// 入口参数: addr: (PWMD特殊功能寄存器的低7位)
//           dat: (待写入的数据)
// 函数返回: 无
////////////////////////////////////////;
void HSPWMD_WriteReg(uint8_t addr, uint8_t dat)
{
    while (HSPWMD_CheckBusy());         //等待前一个异步读写完成
    HSPWMD_AsyncWrite(addr, dat);       //触发异步方式写寄存器
}

////////////////////////////////////////
// 异步方式读取PWME特殊功能寄存器
// 入口参数: addr: (PWME特殊功能寄存器的低7位)
// 函数返回: 寄存器的值
////////////////////////////////////////;
uint8_t HSPWME_ReadReg(uint8_t addr)
{
    uint8_t dat;

    while (HSPWME_CheckBusy());         //等待前一个异步读写完成
    HSPWME_AsyncRead(addr, dat);        //触发异步方式读取寄存器

    return dat;
}

////////////////////////////////////////
// 异步方式写PWME特殊功能寄存器
// 入口参数: addr: (PWME特殊功能寄存器的低7位)
//           dat: (待写入的数据)
// 函数返回: 无
////////////////////////////////////////;
void HSPWME_WriteReg(uint8_t addr, uint8_t dat)
{
    while (HSPWME_CheckBusy());         //等待前一个异步读写完成
    HSPWME_AsyncWrite(addr, dat);       //触发异步方式写寄存器
}

////////////////////////////////////////
// 异步方式读取PWMF特殊功能寄存器
// 入口参数: addr: (PWMF特殊功能寄存器的低7位)
// 函数返回: 寄存器的值
////////////////////////////////////////;
uint8_t HSPWMF_ReadReg(uint8_t addr)
{
    uint8_t dat;

    while (HSPWMF_CheckBusy());         //等待前一个异步读写完成
    HSPWMF_AsyncRead(addr, dat);        //触发异步方式读取寄存器

    return dat;
}

////////////////////////////////////////
// 异步方式写PWMF特殊功能寄存器
// 入口参数: addr: (PWMF特殊功能寄存器的低7位)
//           dat: (待写入的数据)
// 函数返回: 无
////////////////////////////////////////;
void HSPWMF_WriteReg(uint8_t addr, uint8_t dat)
{
    while (HSPWMF_CheckBusy());         //等待前一个异步读写完成
    HSPWMF_AsyncWrite(addr, dat);       //触发异步方式写寄存器
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 定义符合 0x16 (CCR5X基地址) 的4字节结构
typedef struct {
    uint8_t CCR5X;     // 填0
    uint8_t Reserved;  // 填0
    uint8_t CCR5H;     // 填0 (你的值<256, 高位为0)
    uint8_t CCR5L;     // 【关键】占空比数值
} DMA_Burst4_Packet;

// 定义缓冲区：发送3个数据 (0码, 1码, 结束码)
// 3个数据点 * 4字节 = 12字节
DMA_Burst4_Packet xdata TestBuffer[3]; 

void PWM_Test_Simple(void)
{
    // --- 1. 填充测试数据 ---
    
    // 第1个波形：发送 "0码" (CCR = 18)
    TestBuffer[0].CCR5X = 0;
    TestBuffer[0].Reserved = 0;
    TestBuffer[0].CCR5H = 0;
    TestBuffer[0].CCR5L = 18;  // 理论高电平约 300ns

    // 第2个波形：发送 "1码" (CCR = 45)
    TestBuffer[1].CCR5X = 0;
    TestBuffer[1].Reserved = 0;
    TestBuffer[1].CCR5H = 0;
    TestBuffer[1].CCR5L = 45;  // 理论高电平约 750ns

    // 第3个波形：全0 (用于拉低总线，作为结束)
    TestBuffer[2].CCR5X = 0;
    TestBuffer[2].Reserved = 0;
    TestBuffer[2].CCR5H = 0;
    TestBuffer[2].CCR5L = 0;   // 占空比0，强制低电平

    // --- 2. 这里的初始化需确保使用了你调整后的参数 ---
    // (假设 PWME_Init 已经调用，且分频=1, ARR=74)
    
    // 强制更新一下关键的DMA配置，防止Init里没改对
   // PWME_SetDMABaseAddress(22);     // 目标地址 0x16
    PWME_SetDMABurst4Bytes();        // 【核心】必须是4字节突发
    
    // --- 3. 启动DMA发送 ---
    DMA_PWME_DisableTx();
    DMA_PWME_SetTxAddress((uint8_t *)TestBuffer); // 强转指针
    DMA_PWME_SetTxAmount(12);     // 发送 12 个字节
    DMA_PWME_ClearTxFlag();
    DMA_PWME_EnableTx();
    DMA_PWME_TriggerTx();
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


