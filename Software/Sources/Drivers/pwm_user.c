#include "pwm_user.h"

#define PWMC_DMATXSIZE 256
#define PWME_DMATXSIZE 256

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

void PWME_Init(){
  //  PWMC_C3SwitchP6465();               //设置PWMC通道3数据端口: PWMC3P (P6.4), PWMC3N (P6.5)
    PWME_C5SwitchP50();                 //设置PWME引脚切换选项5: P5.0
    HSPWME_Disable();                   //关闭高速模式，使用同步方式进行初始化
    PWME_InternalClockMode();           //选择内部时钟模式
    PWME_SetClockDivider(0);            //设置16位预分频
    PWME_SetReload16(149);              //设置16位重载值
    PWME_BufferARR();                   //使能重载值寄存器预装载功能
    PWME_SetCounter(0);                 //初始化计数值

    PWME_CC5PDisable();                 //关闭通道
  //  PWME_CC5Output();                   //使能通道输出功能
    PWME_CC5PEnable();                  //打开通道
    PWME_SetCCR5Value(36);
    //PWM_UpdateDuty(PWME_CH3, 500);      //设置通道的PWM占空比
    PWME_OC5REFPWMMode1();              //设置通道输出参考信号为PWM模式1信号
    PWME_BufferCCR5();                  //使能CCRn预装载功能
    PWME_CC5PNonInverted();             //设置正极通道输出高电平有效
    PWME_EnablePWM5POutput();           //使能正极通道输出
   // PWME_DisablePWM3NOutput();          //关闭负极通道输出

    PWME_EnableMainOutput();            //使能PWM主输出

    PWME_UpdateIntAsDMAEvent();         //设置PWME更新中断作为DMA事件
    PWME_SetDMABaseAddress(0x16);       //设置PWME DMA的寄存器基地址 (CCR5: CCR5X,Reserved,CCR5H,CCR5L)
    PWME_SetDMABurstCount(0);           //设置PWME DMA突发传输次数
    PWME_SetDMABurst4Bytes();           //设置PWME DMA突发传输字节数
    PWME_DMASkipSpace();                //设置PWME DMA时跳过保留寄存器（空白字节）
    PWME_SetDMAOutput();                //设置PWME DMA方向为输出（DMA输出PWM波形）
   // DMA_SPI1_SetTxAddress(d)        (DMA_SPI_TXAH = HIBYTE(d), DMA_SPI_TXAL = LOBYTE(d))
 //   DMA_PWMET_TXAH = HIBYTE(pu8PWMEDMATxBuffer);  //设置PWME发送DMA总字节数
 //   DMA_PWMET_TXAL = LOBYTE(pu8PWMEDMATxBuffer);

  //  DMA_PWMER_AMTH = HIBYTE(PWME_DMATXSIZE - 1); //设置PWME发送DMA总字节数
  //  DMA_PWMER_AMT = LOBYTE(PWME_DMATXSIZE - 1 ); //设置PWME发送DMA总字节数
    DMA_PWME_SetTxAmount(PWME_DMATXSIZE - 1); //设置PWME发送DMA总字节数
  //  DMA_PWME_SetTxAddress(pu8PWMEDMATxBuffer); //设置PWME发送DMA缓冲区地址
    DMA_PWME_SetInterval(0);  //设置PWME发送DMA字节间隔时间（系统时钟）
    DMA_PWME_ClearTxFlag();             //清除PWME发送DMA中断标志
    DMA_PWME_SetTxBusPriority(0);       //设置总线访问为最低优先级
    //DMA_PWME_SetTxIntPriority(0);       //设置中断为最低优先级
    DMA_PWMER_CFG &= ~(0x03);           //最低优先级
    DMA_PWME_EnableTxInt();             //使能PWME发送DMA中断
    DMA_PWME_EnableTx();                //使能PWME发送DMA功能
    PWME_EnableDMA();                   //设置PWME DMA方向为输出（DMA输出PWM波形）
    PWME_DMANeverSkipSpace();
    PWME_Run();                         //PWME开始运行
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