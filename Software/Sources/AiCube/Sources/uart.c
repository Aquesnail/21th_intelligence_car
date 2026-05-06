//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: uart.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-01-18) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t u8UART2RxCount;
uint8_t xdata pu8UART2RxBuffer[UART2_RXSIZE]; //串口2数据接收缓冲区数组
uint8_t xdata pu8UR2DMATxBuffer[UART2_DMATXSIZE]; //串口2DMA发送缓冲区数组
uint8_t xdata pu8UR2DMARxBuffer[UART2_DMARXSIZE]; //串口2DMA接收缓冲区数组
uint8_t u8UART4RxCount;
uint8_t xdata pu8UART4RxBuffer[UART4_RXSIZE]; //串口4数据接收缓冲区数组

////////////////////////////////////////
// 串口2初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART2_Init(void)
{
#ifdef BAUDRATE
#undef BAUDRATE
#endif
#define BAUDRATE                (115200UL)
#define T2_RELOAD               (65536 - (SYSCLK / BAUDRATE + 2) / 4)

    UART2_SwitchP1011();                //设置串口数据端口: RxD2 (P1.0), TxD2 (P1.1)

    UART2_Timer2BRT();                  //串口2固定选择定时器2作为波特率发生器（W1[S2CFG.0]必须设置为1）
    TIMER2_TimerMode();                 //设置定时器2为定时模式
    TIMER2_1TMode();                    //设置定时器2为1T模式
    TIMER2_SetPrescale(0);              //设置定时器2的8位预分频
    TIMER2_SetReload16(T2_RELOAD);      //设置定时器2的16位重载值
    TIMER2_Run();                       //定时器2开始运行

    UART2_EnableRx();                   //使能串口2接收数据
    UART2_Mode1();                      //设置串口2为模式1 (8位数据可变波特率)

    UART2_TimeoutScale_BRT();           //设置串口波特率为接收超时时间基准
    UART2_SetTimeoutInterval(0x00000a); //设置接收超时时间长度
    UART2_EnableTimeout();              //使能串口接收超时功能

    DMA_UART2_SetTxAmount(UART2_DMATXSIZE - 1); //设置串口DMA发送总字节数
    DMA_UART2_SetTxAddress(pu8UR2DMATxBuffer); //设置串口DMA发送缓冲区地址
    DMA_UART2_SetInterval(UART2_DMAITV); //设置串口DMA发送/接收字节间隔时间（系统时钟）
    DMA_UART2_ClearTxFlag();            //清除串口发送DMA中断标志
    DMA_UART2_SetTxBusPriority(0);      //设置总线访问为最低优先级
    DMA_UART2_SetTxIntPriority(0);      //设置中断为最低优先级
    DMA_UART2_EnableTxInt();            //使能串口2DMA发送中断
    DMA_UART2_EnableTx();               //使能串口2DMA发送功能
//  DMA_UART2_TriggerTx();              //触发串口2DMA发送

    DMA_UART2_SetRxAmount(UART2_DMARXSIZE - 1); //设置串口DMA接收总字节数
    DMA_UART2_SetRxAddress(pu8UR2DMARxBuffer); //设置串口DMA接收缓冲区地址
    DMA_UART2_SetInterval(UART2_DMAITV); //设置串口DMA发送/接收字节间隔时间（系统时钟）
    DMA_UART2_ClearFIFO();              //清空串口DMA接收FIFO缓冲区
    DMA_UART2_ClearRxFlag();            //清除串口接收DMA中断标志
    DMA_UART2_SetRxBusPriority(0);      //设置总线访问为最低优先级
    DMA_UART2_SetRxIntPriority(0);      //设置中断为最低优先级
    DMA_UART2_EnableRxInt();            //使能串口2DMA接收中断
    DMA_UART2_EnableRx();               //使能串口2DMA接收功能
//  DMA_UART2_TriggerRx();              //触发串口2DMA接收

    UART2_SetTxFlag();                  //设置发送标志以提高发送效率
    u8UART2RxCount = 0;                 //初始化接收字节数量

    //<<AICUBE_USER_UART2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_UART2_INITIAL_END>>
}

////////////////////////////////////////
// 串口2发送数据函数
// 入口参数: dat (待发送的字节数据)
// 函数返回: 无
////////////////////////////////////////
void UART2_SendByte(uint8_t dat)
{
    while (!UART2_CheckTxFlag());       //等待之前的数据发送完成
    UART2_ClearTxFlag();                //清除完成标志
    UART2_SendData(dat);                //发送当前字节
}

////////////////////////////////////////
// 串口2发送多字节数据函数
// 入口参数: dat  (发送数据缓冲区)
//           size (数据大小)
// 函数返回: 无
////////////////////////////////////////
void UART2_SendBuffer(uint8_t *dat, uint8_t size)
{
    while (size--)                      //判断数据是否结束
        UART2_SendByte(*dat++);         //发送当前字节
}

////////////////////////////////////////
// 串口2发送字符串函数
// 入口参数: str  (字符串首地址)
// 函数返回: 无
////////////////////////////////////////
void UART2_SendString(char *str)
{
    char c;

    while (c = *str++)                  //判断字符串是否结束
        UART2_SendByte(c);              //发送当前字节
}

////////////////////////////////////////
// 串口2轮询接收数据函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART2_RecvPolling(void)
{
    if (!UART2_CheckRxFlag())           //判断串口接收中断
        return;
    UART2_ClearRxFlag();                //清除串口接收中断标志

    pu8UART2RxBuffer[u8UART2RxCount++] = UART2_ReadData(); //保存接收的数据到接收缓冲区
    if (u8UART2RxCount >= UART2_RXSIZE)
        u8UART2RxCount = 0;             //防止数组边界溢出
}

////////////////////////////////////////
// 串口4初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART4_Init(void)
{
#ifdef BAUDRATE
#undef BAUDRATE
#endif
#define BAUDRATE                (115200UL)
#define T2_RELOAD               (65536 - (SYSCLK / BAUDRATE + 2) / 4)

    UART4_SwitchP5253();                //设置串口数据端口: RxD4 (P5.2), TxD4 (P5.3)

    UART4_Timer2BRT();                  //选择定时器2作为串口4波特率发生器
    TIMER2_TimerMode();                 //设置定时器2为定时模式
    TIMER2_1TMode();                    //设置定时器2为1T模式
    TIMER2_SetPrescale(0);              //设置定时器2的8位预分频
    TIMER2_SetReload16(T2_RELOAD);      //设置定时器2的16位重载值
    TIMER2_Run();                       //定时器2开始运行

    UART4_EnableRx();                   //使能串口4接收数据
    UART4_Mode1();                      //设置串口4为模式1 (8位数据可变波特率)

    UART4_SetTxFlag();                  //设置发送标志以提高发送效率
    u8UART4RxCount = 0;                 //初始化接收字节数量

    //<<AICUBE_USER_UART4_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_UART4_INITIAL_END>>
}

////////////////////////////////////////
// 串口4发送数据函数
// 入口参数: dat (待发送的字节数据)
// 函数返回: 无
////////////////////////////////////////
void UART4_SendByte(uint8_t dat)
{
    while (!UART4_CheckTxFlag());       //等待之前的数据发送完成
    UART4_ClearTxFlag();                //清除完成标志
    UART4_SendData(dat);                //发送当前字节
}

////////////////////////////////////////
// 串口4发送多字节数据函数
// 入口参数: dat  (发送数据缓冲区)
//           size (数据大小)
// 函数返回: 无
////////////////////////////////////////
void UART4_SendBuffer(uint8_t *dat, uint8_t size)
{
    while (size--)                      //判断数据是否结束
        UART4_SendByte(*dat++);         //发送当前字节
}

////////////////////////////////////////
// 串口4发送字符串函数
// 入口参数: str  (字符串首地址)
// 函数返回: 无
////////////////////////////////////////
void UART4_SendString(char *str)
{
    char c;

    while (c = *str++)                  //判断字符串是否结束
        UART4_SendByte(c);              //发送当前字节
}

////////////////////////////////////////
// 串口4轮询接收数据函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART4_RecvPolling(void)
{
    if (!UART4_CheckRxFlag())           //判断串口接收中断
        return;
    UART4_ClearRxFlag();                //清除串口接收中断标志

    pu8UART4RxBuffer[u8UART4RxCount++] = UART4_ReadData(); //保存接收的数据到接收缓冲区
    if (u8UART4RxCount >= UART4_RXSIZE)
        u8UART4RxCount = 0;             //防止数组边界溢出
}


////////////////////////////////////////
// 串口2发送DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_UART2TX_ISR(void) interrupt DMA_UR2T_VECTOR
{
    //<<AICUBE_USER_UART2_ISR_CODE2_BEGIN>>
    // 在此添加中断函数用户代码  
    if (DMA_UART2_CheckTxFlag())        //判断串口发送DMA中断
    {
        DMA_UART2_ClearTxFlag();        //清除串口发送DMA中断标志
    }
    //<<AICUBE_USER_UART2_ISR_CODE2_END>>
}

////////////////////////////////////////
// 串口2接收DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_UART2RX_ISR(void) interrupt DMA_UR2R_VECTOR
{
    //<<AICUBE_USER_UART2_ISR_CODE3_BEGIN>>
    // 在此添加中断函数用户代码  
    if (DMA_UART2_CheckRxFlag())        //判断串口接收DMA中断
    {
        DMA_UART2_ClearRxFlag();        //清除串口接收DMA中断标志
    }
    //<<AICUBE_USER_UART2_ISR_CODE3_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
uint8_t UART5_Write(char* str,uint16_t len){
	return 0;

}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


