//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: spi.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-02-14) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t xdata pu8SPI2DMATxBuffer[16];   //SPI2 DMA发送缓冲区数组
uint8_t xdata pu8SPI2DMARxBuffer[16];   //SPI2 DMA接收缓冲区数组

////////////////////////////////////////
// SPI2初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void SPI2_Init(void)
{
    SPI2_SwitchPBn();                   //选择SPI数据口: SS(PB.1), MOSI(PB.2), MISO(PB.3), SCLK(PB.4)

    SPI2_MasterMode();                  //设置SPI2为主机模式
    SPI2_IgnoreSS();                    //忽略SS脚
    SPI2_DataMSB();                     //设置SPI2数据顺序为MSB (高位在前)
    SPI2_SetMode0();                    //设置SPI2工作模式0 (CPOL=0, CPHA=0)
    SPI2_SetClockDivider16();           //设置SPI2时钟分频

    HSSPI2_Disable();                   //关闭SPI2高速模式

    SPI2_Enable();                      //使能SPI2功能

    DMA_SPI2_ManualSS();                //关闭SPI2 DMA自动控制SS功能
    DMA_SPI2_SetAmount(15);             //设置SPI2 DMA发送/接收字节数
    DMA_SPI2_SetTxAddress(pu8SPI2DMATxBuffer); //设置SPI2 DMA发送缓冲区地址
    DMA_SPI2_SetRxAddress(pu8SPI2DMARxBuffer); //设置SPI2 DMA接收缓冲区地址
    DMA_SPI2_SetInterval(0);            //设置SPI2 DMA发送/接收字节间隔时间（系统时钟）
    DMA_SPI2_ClearFIFO();               //清空SPI2 DMA FIFO缓冲区
    DMA_SPI2_ClearFlag();               //清除SPI2 DMA中断标志
    DMA_SPI2_EnableTx();                //使能发送数据
    DMA_SPI2_EnableRx();                //使能接收数据
    DMA_SPI2_SetBusPriority(0);         //设置总线访问为最低优先级
    DMA_SPI2_SetIntPriority(0);         //设置中断为最低优先级
    DMA_SPI2_EnableInt();               //使能SPI2 DMA中断
    DMA_SPI2_Enable();                  //使能SPI2 DMA功能
//  DMA_SPI2_MasterTrigger();           //触发SPI2主机DMA

    //<<AICUBE_USER_SPI2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_SPI2_INITIAL_END>>
}

////////////////////////////////////////
// SPI2主机模式发送字节函数
// 入口参数: dat (待发送的字节数据)
// 函数返回: 无
////////////////////////////////////////
void SPI2_WriteByte(uint8_t dat)
{
    SPI2_SendData(dat);                 //触发主机发送数据
    while (!SPI2_CheckFlag());          //等待发送完成
    SPI2_ClearFlag();                   //清除中断标志
}

////////////////////////////////////////
// SPI2主机模式读取字节函数
// 入口参数: 无
// 函数返回: 读取的字节数据
////////////////////////////////////////
uint8_t SPI2_ReadByte(void)
{
    SPI2_SendData(0xff);                //触发主机读取数据（主机发送时钟信号）
    while (!SPI2_CheckFlag());          //等待读取完成
    SPI2_ClearFlag();                   //清除中断标志
    return SPI2_ReadData();
}


////////////////////////////////////////
// SPI2 DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_SPI2_ISR(void) interrupt DMA_SPI2_VECTOR
{
    //<<AICUBE_USER_SPI2_ISR_CODE2_BEGIN>>
    // 在此添加中断函数用户代码  
    if (DMA_SPI2_CheckFlag())           //判断SPI2 DMA中断
    {
        DMA_SPI2_ClearFlag();           //清除SPI2 DMA中断标志
    }
    //<<AICUBE_USER_SPI2_ISR_CODE2_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


