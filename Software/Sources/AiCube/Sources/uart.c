//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: uart.c
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


uint8_t xdata pu8UR5DMATxBuffer[16];    //串口5DMA发送缓冲区数组
uint8_t xdata pu8UR5DMARxBuffer[16];    //串口5DMA接收缓冲区数组

////////////////////////////////////////
// 串口5初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART5_Init(void)
{
#ifdef BAUDRATE
#undef BAUDRATE
#endif
#define BAUDRATE                (115200)
#define T2_RELOAD               (65536 - (SYSCLK / BAUDRATE + 2) / 4)

    UART5_SwitchP2021();                //设置串口数据端口: RxD5 (P2.0), TxD5 (P2.1)

    UART5_Timer2BRT();                  //选择定时器2作为串口5波特率发生器
    TIMER2_TimerMode();                 //设置定时器2为定时模式
    TIMER2_1TMode();                    //设置定时器2为1T模式
    TIMER2_SetPrescale(0);              //设置定时器2的8位预分频
    TIMER2_SetReload16(T2_RELOAD);      //设置定时器2的16位重载值
    TIMER2_Run();                       //定时器2开始运行

    UART5_EnableRx();                   //使能串口5接收数据
    UART5_Mode1();                      //设置串口5为模式1 (8位数据可变波特率)
    UART5_SetIntPriority(1);            //设置中断为较低优先级
    UART5_EnableInt();                  //使能串口5中断

    UART5_TimeoutScale_BRT();           //设置串口波特率为接收超时时间基准
    UART5_SetTimeoutInterval(0x00000a); //设置接收超时时间长度
    UART5_EnableTimeoutInt();           //使能串口接收超时中断
    UART5_EnableTimeout();              //使能串口接收超时功能

    DMA_UART5_SetTxAmount(15);          //设置串口DMA发送总字节数
    DMA_UART5_SetTxAddress(pu8UR5DMATxBuffer); //设置串口DMA发送缓冲区地址
    DMA_UART5_SetInterval(0);           //设置串口DMA发送/接收字节间隔时间（系统时钟）
    DMA_UART5_ClearTxFlag();            //清除串口发送DMA中断标志
    DMA_UART5_SetTxBusPriority(0);      //设置总线访问为最低优先级
    DMA_UART5_SetTxIntPriority(1);      //设置中断为较低优先级
    DMA_UART5_EnableTxInt();            //使能串口5DMA发送中断
    DMA_UART5_EnableTx();               //使能串口5DMA发送功能
//  DMA_UART5_TriggerTx();              //触发串口5DMA发送

    DMA_UART5_SetRxAmount(15);          //设置串口DMA接收总字节数
    DMA_UART5_SetRxAddress(pu8UR5DMARxBuffer); //设置串口DMA接收缓冲区地址
    DMA_UART5_SetInterval(0);           //设置串口DMA发送/接收字节间隔时间（系统时钟）
    DMA_UART5_ClearFIFO();              //清空串口DMA接收FIFO缓冲区
    DMA_UART5_ClearRxFlag();            //清除串口接收DMA中断标志
    DMA_UART5_SetRxBusPriority(1);      //设置总线访问为较低优先级
    DMA_UART5_SetRxIntPriority(0);      //设置中断为最低优先级
    DMA_UART5_EnableRxInt();            //使能串口5DMA接收中断
    DMA_UART5_EnableRx();               //使能串口5DMA接收功能
//  DMA_UART5_TriggerRx();              //触发串口5DMA接收

    //<<AICUBE_USER_UART5_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_UART5_INITIAL_END>>
}


////////////////////////////////////////
// 串口5中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART5_ISR(void) interrupt UART5_VECTOR
{
    //<<AICUBE_USER_UART5_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    if (UART5_CheckTxFlag())            //判断串口发送中断
    {
        UART5_ClearTxFlag();            //清除串口发送中断标志
    }

    if (UART5_CheckRxFlag())            //判断串口接收中断
    {
        UART5_ClearRxFlag();            //清除串口接收中断标志
    }

    if (UART5_CheckTimeoutFlag())       //判断串口接收超时中断
    {
        UART5_ClearTimeoutFlag();       //清除串口接收超时中断标志
    }
    //<<AICUBE_USER_UART5_ISR_CODE1_END>>
}

////////////////////////////////////////
// 串口5发送DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_UART5TX_ISR(void) interrupt DMA_UR5T_VECTOR
{
    //<<AICUBE_USER_UART5_ISR_CODE2_BEGIN>>
    // 在此添加中断函数用户代码  
    if (DMA_UART5_CheckTxFlag())        //判断串口发送DMA中断
    {
        DMA_UART5_ClearTxFlag();        //清除串口发送DMA中断标志
    }
    //<<AICUBE_USER_UART5_ISR_CODE2_END>>
}

////////////////////////////////////////
// 串口5接收DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_UART5RX_ISR(void) interrupt DMA_UR5R_VECTOR
{
    //<<AICUBE_USER_UART5_ISR_CODE3_BEGIN>>
    // 在此添加中断函数用户代码  
    if (DMA_UART5_CheckRxFlag())        //判断串口接收DMA中断
    {
        DMA_UART5_ClearRxFlag();        //清除串口接收DMA中断标志
    }
    //<<AICUBE_USER_UART5_ISR_CODE3_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


