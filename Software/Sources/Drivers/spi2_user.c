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

#include "spi2_user.h"
#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portasm.h"
#include "string.h"

//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明

// SPI2 DMA传输完成标志（用于阻塞函数）
volatile uint8_t dma_spi2_done;

// 以下变量用于非阻塞DMA传输
static uint8_t *spi2_rx_buffer;         // 用户接收缓冲区指针
static uint8_t spi2_transfer_len;        // 传输长度
static volatile uint8_t spi2_dma_busy;   // 非阻塞传输忙标志


static DMA_SPI2_Callback_t spi2_dma_callback = (void *)NULL;

//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t xdata pu8SPI2DMATxBuffer[512];   //SPI2 DMA发送缓冲区数组
uint8_t xdata pu8SPI2DMARxBuffer[16];   //SPI2 DMA接收缓冲区数组

////////////////////////////////////////
// SPI2初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void SPI2_Init(DMA_SPI2_Callback_t callback)
{
    SPI2_SwitchPBn();                   //选择SPI数据口: SS(P8.0), MOSI(P8.1), MISO(P8.2), SCLK(P8.3)

    SPI2_MasterMode();                  //设置SPI2为主机模式
    SPI2_IgnoreSS();                    //忽略SS脚
    SPI2_DataMSB();                     //设置SPI2数据顺序为MSB (高位在前)
    SPI2_SetMode0();                    //设置SPI2工作模式0 (CPOL=0, CPHA=0)
    SPI2_SetClockDivider16();           //设置SPI2时钟分频

    HSSPI2_Disable();                   //关闭SPI2高速模式

    SPI2_Enable();                      //使能SPI2功能

    DMA_SPI2_ManualSS();                //关闭SPI2 DMA自动控制SS功能
   // DMA_SPI2_SetAmount(15);             //设置SPI2 DMA发送/接收字节数
    DMA_SPI2_SetTxAddress(pu8SPI2DMATxBuffer); //设置SPI2 DMA发送缓冲区地址
    DMA_SPI2_SetRxAddress(pu8SPI2DMARxBuffer); //设置SPI2 DMA接收缓冲区地址
    DMA_SPI2_SetInterval(0);            //设置SPI2 DMA发送/接收字节间隔时间（系统时钟）
    DMA_SPI2_ClearFIFO();               //清空SPI2 DMA FIFO缓冲区
    DMA_SPI2_ClearFlag();               //清除SPI2 DMA中断标志
    DMA_SPI2_EnableTx();                //使能发送数据
    DMA_SPI2_EnableRx();                //使能接收数据
    DMA_SPI2_SetBusPriority(0);         //设置总线访问为最低优先级
    DMA_SPI2_SetIntPriority(1);         //设置中断为较低优先级
    DMA_SPI2_EnableInt();               //使能SPI2 DMA中断
    DMA_SPI2_Enable();                  //使能SPI2 DMA功能
//  DMA_SPI2_MasterTrigger();           //触发SPI2主机DMA

    //<<AICUBE_USER_SPI2_INITIAL_BEGIN>>
     // 填充发送缓冲区为0xFF，用于产生时钟
    memset(pu8SPI2DMATxBuffer,0xFF,sizeof(pu8SPI2DMATxBuffer));
    spi2_dma_callback = callback;
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

uint8_t SPI2_WriteReadByte(uint8_t dat)
{
    SPI2_SendData(dat);                //触发主机读取数据（主机发送时钟信号）
    while (!SPI2_CheckFlag());          //等待读取完成
    SPI2_ClearFlag();                   //清除中断标志
    return SPI2_ReadData();
}

////////////////////////////////////////
// SPI2 DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_SPI2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    if (DMA_SPI2_CheckFlag())           //判断SPI2 DMA中断
    {
        DMA_SPI2_ClearFlag();           //清除SPI2 DMA中断标志s
        // 处理非阻塞传输回调
        if (spi2_dma_callback != 0) {
            // 将DMA接收缓冲区的数据拷贝到用户缓冲区
            spi2_dma_callback(xHigherPriorityTaskWoken);          // 调用用户回调
        }
        // 设置完成标志（用于阻塞函数）
        dma_spi2_done = 1;
        spi2_dma_busy = 0;              
        // 清除非阻塞忙标志
    }
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码

/******************************************************************************
 * @brief    SPI2 DMA多字节非阻塞读取
 * @param    rx_buf   - 存放读取数据的缓冲区指针（长度至少为len）
 * @param    len      - 要读取的字节数（不超过16）
 * @param    callback - 传输完成后的回调函数（可为NULL）
 * @return   无
 * @note     函数立即返回，传输完成后自动调用回调函数（在中断中执行）。
 *           传输期间请勿再次调用本函数，可通过spi2_dma_busy标志判断是否忙。
 ******************************************************************************/
void SPI2_DMA_ReadBytes_IT(uint8_t xdata*rx_buf, uint16_t len)
{
    // 参数检查

    
    // 等待之前的非阻塞传输完成（简单轮询，也可改为返回错误）
    while (spi2_dma_busy);

    // 保存用户参数
    spi2_rx_buffer = rx_buf;
    spi2_transfer_len = len;
    

    // 配置DMA传输指定长度
    DMA_SPI2_SetAmount(len);
    DMA_SPI2_SetTxAddress(pu8SPI2DMATxBuffer);
    DMA_SPI2_SetRxAddress(rx_buf);


    // 启动DMA传输（非阻塞）
    spi2_dma_busy = 1;          // 设置忙标志
    DMA_SPI2_MasterTrigger();   // 触发主机DMA传输
    // 函数立即返回，传输完成后进入中断并调用回调
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


