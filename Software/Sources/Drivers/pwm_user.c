#include "pwm_user.h"

#define PWMC_DMATXSIZE 256

//uint8_t xdata pu8PWMCDMATxBuffer[PWMC_DMATXSIZE]; //PWMCDMA发送缓冲区数组

void PWMC_Init(void)
{
    PWMC_C3SwitchP6465();               //设置PWMC通道3数据端口: PWMC3P (P6.4), PWMC3N (P6.5)

    HSPWMC_Disable();                   //关闭高速模式，使用同步方式进行初始化
    PWMC_InternalClockMode();           //选择内部时钟模式
    PWMC_SetClockDivider(0);            //设置16位预分频
    PWMC_SetReload16(149);              //设置16位重载值
    PWMC_BufferARR();                   //使能重载值寄存器预装载功能
    PWMC_SetCounter(0);                 //初始化计数值

    PWMC_CC3PDisable();                 //关闭通道
    PWMC_CC3Output();                   //使能通道输出功能
    PWMC_CC3PEnable();                  //打开通道
    PWMC_SetCCR3Value(36);  
    //PWM_UpdateDuty(PWMC_CH3, 500);      //设置通道的PWM占空比
    PWMC_OC3REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWMC_BufferCCR3();                  //使能CCRn预装载功能
    PWMC_CC3PNonInverted();             //设置正极通道输出高电平有效
    PWMC_EnablePWM3POutput();           //使能正极通道输出
    PWMC_DisablePWM3NOutput();          //关闭负极通道输出

    PWMC_EnableMainOutput();            //使能PWM主输出

    PWMC_UpdateIntAsDMAEvent();         //设置PWMC更新中断作为DMA事件
    PWMC_SetDMABaseAddress(15);         //设置PWMC DMA的寄存器基地址
    PWMC_SetDMABurstCount(0);           //设置PWMC DMA突发传输次数
    PWMC_SetDMABurst4Bytes();           //设置PWMC DMA突发传输字节数
    PWMC_DMASkipSpace();                //设置PWMC DMA时跳过保留寄存器（空白字节）
    PWMC_SetDMAOutput();                //设置PWMC DMA方向为输出（DMA输出PWM波形）
   // DMA_SPI1_SetTxAddress(d)        (DMA_SPI_TXAH = HIBYTE(d), DMA_SPI_TXAL = LOBYTE(d))
 //   DMA_PWMCT_TXAH = HIBYTE(pu8PWMCDMATxBuffer);  //设置PWMC发送DMA总字节数
 //   DMA_PWMCT_TXAL = LOBYTE(pu8PWMCDMATxBuffer);

  //  DMA_PWMCR_AMTH = HIBYTE(PWMC_DMATXSIZE - 1); //设置PWMC发送DMA总字节数
  //  DMA_PWMCR_AMT = LOBYTE(PWMC_DMATXSIZE - 1 ); //设置PWMC发送DMA总字节数
    DMA_PWMC_SetTxAmount(PWMC_DMATXSIZE - 1); //设置PWMC发送DMA总字节数
  //  DMA_PWMC_SetTxAddress(pu8PWMCDMATxBuffer); //设置PWMC发送DMA缓冲区地址
    DMA_PWMC_SetInterval(0);  //设置PWMC发送DMA字节间隔时间（系统时钟）
    DMA_PWMC_ClearTxFlag();             //清除PWMC发送DMA中断标志
    DMA_PWMC_SetTxBusPriority(0);       //设置总线访问为最低优先级
    //DMA_PWMC_SetTxIntPriority(0);       //设置中断为最低优先级
    DMA_PWMCR_CFG &= ~(0x03);           //最低优先级
    DMA_PWMC_EnableTxInt();             //使能PWMC发送DMA中断
    DMA_PWMC_EnableTx();                //使能PWMC发送DMA功能
    PWMC_EnableDMA();                   //设置PWMC DMA方向为输出（DMA输出PWM波形）
    PWMC_DMANeverSkipSpace();
    PWMC_Run();                         //PWMC开始运行
//  DMA_PWMC_TriggerTx();               //触发PWMC发送DMA

    //<<AICUBE_USER_PWM2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PWM2_INITIAL_END>>
}


#define PWMA_CH1                0x11
#define PWMA_CH2                0x12
#define PWMA_CH3                0x13
#define PWMA_CH4                0x14


void PWMA_Init(void)
{
    PWMA_C4SwitchP1617();               //设置PWMA通道4数据端口: PWMA4P (P1.6), PWMA4N (P1.7)

    HSPWMA_Disable();                   //关闭高速模式，使用同步方式进行初始化
    PWMA_InternalClockMode();           //选择内部时钟模式
    PWMA_SetClockDivider(479);          //设置16位预分频
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

    PWMA_CC4PDisable();                 //关闭通道
    PWMA_CC4Output();                   //使能通道输出功能
    PWMA_CC4PEnable();                  //打开通道
   // PWM_UpdateDuty(PWMA_CH4, 500);      //设置通道的PWM占空比
    PWMA_OC4REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWMA_DisableOC4FastMode();          //关闭通道快速输出
    PWMA_UnbufferCCR4();                //关闭CCRn预装载功能
    PWMA_DisableETRFClearOC4REF();      //通道输出不受ETI信号影响
    PWMA_CC4PNonInverted();             //设置正极通道输出高电平有效
    PWMA_OC4PIdleLevelLow();            //空闲时正极通道输出低电平
    PWMA_EnablePWM4POutput();           //使能正极通道输出
    PWMA_CC4NEnable();                  //打开负极通道
    PWMA_CC4NInverted();                //设置负极通道输出低电平有效
    PWMA_OC4NIdleLevelLow();            //空闲时负极通道输出低电平
    PWMA_EnablePWM4NOutput();           //使能负极通道输出

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
}


void PWMA_Set(uint16_t t){
  if(t>700) t = 0;
  PWMA_SetCCR4Value(t);
}

uint16_t PWMA_Read(void){
  return (PWMA_CCR4H << 8) + PWMA_CCR4L;
}