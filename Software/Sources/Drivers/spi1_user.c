//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息
// 文件名称: spi1_user.c
// 文件描述:
// 文件版本: V1.0
// 修改记录:
//   1. (2026-04-13) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>

#include "spi1_user.h"
#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "string.h"

//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明

// SPI1 DMA传输完成标志（用于阻塞函数）
volatile uint8_t dma_spi1_done = 0;

// 以下变量用于非阻塞DMA传输
static uint8_t *spi1_rx_buffer = NULL;         // 用户接收缓冲区指针
static uint8_t spi1_transfer_len = 0;        // 传输长度
uint8_t spi1_dma_busy = 0;   // 非阻塞传输忙标志


static DMA_SPI1_Callback_t spi1_dma_callback = (void code*)NULL;

//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t xdata pu8SPI1DMATxBuffer[1024] = {0};   //SPI1 DMA发送缓冲区数组

////////////////////////////////////////
// SPI1初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void SPI1_Init(DMA_SPI1_Callback_t callback)
{
    SPI1_SwitchP3n();                  

    SPI1_MasterMode();                  //设置SPI1为主机模式
    SPI1_IgnoreSS();                    //忽略SS脚
    SPI1_DataMSB();                     //设置SPI1数据顺序为MSB (高位在前)
    SPI1_SetMode3();                    //设置SPI1工作模式0 (CPOL=0, CPHA=0)
    SPI1_SetClockDivider8();           //设置SPI1时钟分频

    HSSPI1_Disable();                   //关闭SPI1高速模式

    SPI1_Enable();                      //使能SPI1功能

    DMA_SPI1_ManualSS();                //关闭SPI1 DMA自动控制SS功能
   // DMA_SPI1_SetAmount(15);             //设置SPI1 DMA发送/接收字节数
  //  DMA_SPI1_SetTxAddress(pu8SPI1DMATxBuffer); //设置SPI1 DMA发送缓冲区地址
 //   DMA_SPI1_SetRxAddress(pu8SPI1DMARxBuffer); //设置SPI1 DMA接收缓冲区地址
    DMA_SPI1_SetInterval(0);            //设置SPI1 DMA发送/接收字节间隔时间（系统时钟）
    DMA_SPI1_ClearFIFO();               //清空SPI1 DMA FIFO缓冲区
    DMA_SPI1_ClearFlag();               //清除SPI1 DMA中断标志
    DMA_SPI1_EnableTx();                //使能发送数据
    DMA_SPI1_EnableRx();                //使能接收数据
    DMA_SPI1_SetBusPriority(1);         //设置总线访问为最低优先级
    DMA_SPI1_SetIntPriority(0);         //设置中断为较低优先级
    DMA_SPI1_EnableInt();               //使能SPI1 DMA中断
    DMA_SPI1_Enable();                  //使能SPI1 DMA功能
//  DMA_SPI1_MasterTrigger();           //触发SPI1主机DMA

    //<<AICUBE_USER_SPI1_INITIAL_BEGIN>>
     // 填充发送缓冲区为0xFF，用于产生时钟
    memset(pu8SPI1DMATxBuffer,0xFF,sizeof(pu8SPI1DMATxBuffer));
    spi1_dma_callback = callback;
			spi1_dma_busy = 0;
    //<<AICUBE_USER_SPI1_INITIAL_END>>
}

////////////////////////////////////////
// SPI1主机模式发送字节函数
// 入口参数: dat (待发送的字节数据)
// 函数返回: 无
////////////////////////////////////////
void SPI1_WriteByte(uint8_t dat)
{
    SPI1_SendData(dat);                 //触发主机发送数据
    while (!SPI1_CheckFlag());          //等待发送完成
    SPI1_ClearFlag();                   //清除中断标志
}

////////////////////////////////////////
// SPI1主机模式读取字节函数
// 入口参数: 无
// 函数返回: 读取的字节数据
////////////////////////////////////////
uint8_t SPI1_ReadByte(void)
{
    SPI1_SendData(0xff);                //触发主机读取数据（主机发送时钟信号）
    while (!SPI1_CheckFlag());          //等待读取完成
    SPI1_ClearFlag();                   //清除中断标志
    return SPI1_ReadData();
}

uint8_t SPI1_WriteReadByte(uint8_t dat)
{
    SPI1_SendData(dat);                //触发主机读取数据（主机发送时钟信号）
    while (!SPI1_CheckFlag());          //等待读取完成
    SPI1_ClearFlag();                   //清除中断标志
    return SPI1_ReadData();
}

////////////////////////////////////////
// SPI1 DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_SPI1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken)
{
    if (DMA_SPI1_CheckFlag())           //判断SPI1 DMA中断
    {
        DMA_SPI1_ClearFlag();           //清除SPI1 DMA中断标志s
        // 处理非阻塞传输回调
        if (spi1_dma_callback != 0) {
            // 将DMA接收缓冲区的数据拷贝到用户缓冲区
            spi1_dma_callback(xHigherPriorityTaskWoken);          // 调用用户回调
        }
        // if (xDmaSemaphore != NULL) {
        //     xSemaphoreGiveFromISR(xDmaSemaphore, xHigherPriorityTaskWoken);
        // }
        // 设置完成标志（用于阻塞函数）
        dma_spi1_done = 1;
        spi1_dma_busy = 0;
        // 清除非阻塞忙标志
    }
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码

/******************************************************************************
 * @brief    SPI1 DMA多字节非阻塞读取
 * @param    rx_buf   - 存放读取数据的缓冲区指针（长度至少为len）
 * @param    len      - 要读取的字节数（不超过16）
 * @param    callback - 传输完成后的回调函数（可为NULL）
 * @return   无
 * @note     函数立即返回，传输完成后自动调用回调函数（在中断中执行）。
 *           传输期间请勿再次调用本函数，可通过spi1_dma_busy标志判断是否忙。
 ******************************************************************************/
void SPI1_DMA_ReadBytes_IT(uint8_t xdata*rx_buf, uint16_t len)
{
    // 参数检查


    // 等待之前的非阻塞传输完成（简单轮询，也可改为返回错误）
    if (spi1_dma_busy) return;

    // 保存用户参数
    spi1_rx_buffer = rx_buf;
    spi1_transfer_len = len;


    // 配置DMA传输指定长度
    DMA_SPI1_SetAmount(len);
    DMA_SPI1_SetTxAddress(pu8SPI1DMATxBuffer);
    DMA_SPI1_SetRxAddress(rx_buf);


    // 启动DMA传输（非阻塞）
    spi1_dma_busy = 1;          // 设置忙标志
    DMA_SPI1_MasterTrigger();   // 触发主机DMA传输
    // 函数立即返回，传输完成后进入中断并调用回调
}

void SPI1_DMA_WriteReadBytes_IT(uint8_t xdata*tx_buf,uint8_t xdata*rx_buf, uint16_t len)
{
    // 参数检查


    // 等待之前的非阻塞传输完成（简单轮询，也可改为返回错误）
    while (spi1_dma_busy);

    // 保存用户参数
    spi1_rx_buffer = rx_buf;
    spi1_transfer_len = len;


    // 配置DMA传输指定长度
    DMA_SPI1_SetAmount(len);
    DMA_SPI1_SetTxAddress(tx_buf);
    DMA_SPI1_SetRxAddress(rx_buf);


    // 启动DMA传输（非阻塞）
    spi1_dma_busy = 1;          // 设置忙标志
    DMA_SPI1_MasterTrigger();   // 触发主机DMA传输
    // 函数立即返回，传输完成后进入中断并调用回调
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>
