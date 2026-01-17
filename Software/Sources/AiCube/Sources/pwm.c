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
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



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
    PWMA_SetClockDivider(499);          //设置16位预分频
    PWMA_SetReload16(999);              //设置16位重载值
    PWMA_UnbufferARR();                 //关闭重载值寄存器预装载功能
    PWMA_SetCounter(0);                 //初始化计数值
    PWMA_SetDeadTime(0);                //设置死区时间为：无死区
    PWMA_EdgeAlignMode();               //设置计数器计数为边沿对齐模式
    PWMA_UpCounter();                   //设置计数方向为向上计数
    PWMA_ContinueMode();                //设置计数器为连续计数模式
    PWMA_SetRepeatCounter(0);           //设置重复计数次数
    PWMA_EnableUpdate();                //使能更新事件
    PWMA_UpdateRequestMode2();          //更新事件选择模式2

    PWMA_CC3PDisable();                 //关闭通道
    PWMA_CC3Output();                   //使能通道输出功能
    PWMA_CC3PEnable();                  //打开通道
    PWM_UpdateDuty(PWMA_CH3, 500);      //设置通道的PWM占空比
    PWMA_OC3REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWMA_DisableOC3FastMode();          //关闭通道快速输出
    PWMA_UnbufferCCR3();                //关闭CCRn预装载功能
    PWMA_DisableETRFClearOC3REF();      //通道输出不受ETI信号影响
    PWMA_DisablePWM3POutput();          //关闭正极通道输出
    PWMA_DisablePWM3NOutput();          //关闭负极通道输出

    PWMA_CC4PDisable();                 //关闭通道
    PWMA_CC4Output();                   //使能通道输出功能
    PWMA_CC4PEnable();                  //打开通道
    PWM_UpdateDuty(PWMA_CH4, 500);      //设置通道的PWM占空比
    PWMA_OC4REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWMA_DisableOC4FastMode();          //关闭通道快速输出
    PWMA_UnbufferCCR4();                //关闭CCRn预装载功能
    PWMA_DisableETRFClearOC4REF();      //通道输出不受ETI信号影响
    PWMA_DisablePWM4POutput();          //关闭正极通道输出
    PWMA_DisablePWM4NOutput();          //关闭负极通道输出

    PWMA_StandaloneMode();              //设置PWM定时器为独立模式
    PWMA_UnbufferCCOC();                //捕获/比较控制位直接改写，不预装载
    PWMA_DisableTRGI();                 //关闭触发输入信号
    PWMA_PWM1PToTI();                   //关闭霍尔传感器控制功能
    PWMA_ResetAsTRGO();                 //软件设置UG产生的复位信号作为TRGO
    PWMA_DisableExternalClock2();       //禁止外部时钟模式2
    PWMA_BRKDisable();                  //禁止刹车输入
    PWMA_DisableAutoOutput();           //禁止自动输出功能（只能软件设置MOE）
    PWMA_RunInactiveNoOutput();         //运行模式PWM不工作时禁止输出
    PWMA_IdleInactiveNoOutput();        //空闲模式PWM不工作时禁止输出
    PWMA_EnableMainOutput();            //使能PWM主输出
    PWMA_SetUnLock();                   //寄存器无写保护

    PWMA_Run();                         //PWMA开始运行

    //<<AICUBE_USER_PWM0_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PWM0_INITIAL_END>>
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
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


