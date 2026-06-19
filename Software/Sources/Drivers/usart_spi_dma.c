//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息
// 文件名称: usart_spi_dma.c
// 文件描述: USART1-SPI DMA 驱动, 参照 SPI-DMA.c 提取
//           USART1配置为SPI主机模式, 通过DMA(UART1通道)自动收发
// 文件版本: V1.0
// 修改记录:
//   1. (2026-06-18) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>

#include "usart_spi_dma.h"
#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "string.h"

//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明

// USART1-SPI DMA传输完成标志
volatile uint8_t dma_usart_spi_done = 0;

// 非阻塞DMA传输管理变量
uint8_t usart1_spi_dma_busy = 0;             // 传输忙标志
static DMA_USART_SPI_Callback_t usart1_spi_dma_callback = (void code*)NULL;

//<<AICUBE_USER_GLOBAL_DEFINE_END>>

////////////////////////////////////////
// USART1 SPI模式初始化
// 入口参数: callback - DMA传输完成回调
// 函数返回: 无
// 参照: SPI-DMA.c SPI_init()
// 说明: 将USART1配置为SPI主机模式, 并初始化DMA通道
//   硬件引脚: P1.4(SS), P1.5(MOSI), P1.6(MISO), P1.7(SCLK)
////////////////////////////////////////
void USART1_SPI_Init(DMA_USART_SPI_Callback_t callback)
{
    // ---- USART1 SPI 模式配置 ----
    USART1_SPI_SwitchP1n();             // 切换到 P1.4(SS), P1.5(MOSI), P1.6(MISO), P1.7(SCLK)

    USART1_SPI_MasterMode();            // 设置为主机模式
    USART1_SPI_DataMSB();               // 数据顺序: MSB 高位在前
    USART1_SPI_SetMode3();              // SPI 模式3: CPOL=1, CPHA=1
    USART1_SPI_SetClockDivider16();     // 时钟16分频

    USART1_SPI_ModeEnable();            // 使能 USART1 SPI 模式
    USART1_SPI_Enable();                // 使能 USART1 SPI 功能

    // ---- DMA 通道配置 (参照 SPI-DMA.c usart1_tx_dma 和 usart1_rx_dma) ----
    // TX DMA
    DMA_UR1T_STA = 0x00;                // 清除发送DMA状态
    DMA_UR1T_CFG = 0x80;                // 使能发送DMA中断 (bit7=IE)
    DMA_UART1_SetTxBusPriority(0);      // 总线访问优先级最低
    DMA_UART1_SetTxIntPriority(0);      // 中断优先级最低

    // RX DMA
    DMA_UR1R_STA = 0x00;                // 清除接收DMA状态
    DMA_UR1R_CFG = 0x80;                // 使能接收DMA中断 (bit7=IE)
    DMA_UART1_SetRxBusPriority(0);      // 总线访问优先级最低
    DMA_UART1_SetRxIntPriority(0);      // 中断优先级最低

    // ---- 用户初始化 ----
    //<<AICUBE_USER_SPI1_INITIAL_BEGIN>>
    usart1_spi_dma_callback = callback;
    usart1_spi_dma_busy = 0;
    dma_usart_spi_done = 0;
    //<<AICUBE_USER_SPI1_INITIAL_END>>
}


////////////////////////////////////////
// USART1-SPI DMA中断服务处理
// 入口参数: xHigherPriorityTaskWoken - FreeRTOS任务唤醒标记
// 函数返回: 无
// 参照: SPI-DMA.c UR1T_DMA_Interrupt() / UR1R_DMA_Interrupt()
// 使用: 在中断向量函数中调用此handler
//   void UR1T_DMA_Interrupt(void) interrupt DMA_UR1T_VECTOR
//   {
//       BaseType_t xWoken = pdFALSE;
//       DMA_USART1_SPI_ISR_Handle(&xWoken);
//       portYIELD_FROM_ISR(xWoken);
//   }
////////////////////////////////////////
void DMA_USART1_SPI_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken)
{
    // --- TX DMA 中断处理 (参照 SPI-DMA.c:189-193) ---
    if (DMA_UART1_CheckTxFlag())
    {
        DMA_UART1_ClearTxFlag();
        DMA_UR1T_CR = 0x00;             // 关闭 DMA TX

        dma_usart_spi_done = 1;
        usart1_spi_dma_busy = 0;

        if (usart1_spi_dma_callback != 0) {
            usart1_spi_dma_callback(xHigherPriorityTaskWoken);
        }
    }
    if (DMA_UART1_CheckOverWriteFlag())
    {
        DMA_UART1_ClearOverWriteFlag();
    }

    // --- RX DMA 中断处理 (参照 SPI-DMA.c:195-198) ---
    if (DMA_UART1_CheckRxFlag())
    {
        DMA_UART1_ClearRxFlag();
        DMA_UR1R_CR = 0x00;             // 关闭 DMA RX
    }
    if (DMA_UART1_CheckRxLossFlag())
    {
        DMA_UART1_ClearRxLossFlag();
    }
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码

/******************************************************************************
 * @brief    USART1-SPI DMA发送 (参照 SPI-DMA.c usart1_tx_dma)
 * @param    tx_buf  - 发送数据缓冲区指针 (xdata空间)
 * @param    len     - 发送字节数
 * @return   无
 * @note     非阻塞, 通过回调通知完成
 ******************************************************************************/
void USART1_SPI_DMA_WriteBytes_IT(uint8_t xdata *tx_buf, uint16_t len)
{
    if (usart1_spi_dma_busy) return;

    usart1_spi_dma_busy = 1;

    len--;                              // DMA传输字节数 = 实际值 - 1 (参照 SPI-DMA.c:144)

    DMA_UR1T_STA = 0x00;                // 清除DMA状态 (参照 SPI-DMA.c:146)
    DMA_UR1T_CFG = 0x80;                // 使能DMA中断 (参照 SPI-DMA.c:147)
    DMA_UR1T_AMT = (uint8_t)len;        // 传输字节数低8位 (参照 SPI-DMA.c:148)
    DMA_UR1T_AMTH = (uint8_t)(len >> 8);// 传输字节数高8位 (参照 SPI-DMA.c:149)
    DMA_UR1T_TXAL = (uint8_t)((uint16_t)tx_buf);       // 缓冲区地址低8位 (参照 SPI-DMA.c:150)
    DMA_UR1T_TXAH = (uint8_t)((uint16_t)tx_buf >> 8);  // 缓冲区地址高8位 (参照 SPI-DMA.c:151)
    DMA_UR1T_CR = 0x80;                 // 使能DMA发送 (参照 SPI-DMA.c:152)
}

/******************************************************************************
 * @brief    USART1-SPI DMA接收 (参照 SPI-DMA.c usart1_rx_dma)
 * @param    rx_buf  - 接收数据缓冲区指针 (xdata空间)
 * @param    len     - 接收字节数
 * @return   无
 * @note     非阻塞, 通过回调通知完成
 *           RX需要同时启动TX来提供SPI时钟 (参照 SPI-DMA.c:172)
 ******************************************************************************/
void USART1_SPI_DMA_ReadBytes_IT(uint8_t xdata *rx_buf, uint16_t len)
{
    if (usart1_spi_dma_busy) return;

    usart1_spi_dma_busy = 1;

    len--;                              // DMA传输字节数 = 实际值 - 1 (参照 SPI-DMA.c:162)

    DMA_UR1R_STA = 0x00;                // 清除DMA状态 (参照 SPI-DMA.c:163)
    DMA_UR1R_CFG = 0x00;                // 关闭DMA接收中断 (参照 SPI-DMA.c:164)
    DMA_UR1R_AMT = (uint8_t)len;        // 传输字节数低8位 (参照 SPI-DMA.c:165)
    DMA_UR1R_AMTH = (uint8_t)(len >> 8);// 传输字节数高8位 (参照 SPI-DMA.c:166)
    DMA_UR1R_RXAL = (uint8_t)((uint16_t)rx_buf);       // 缓冲区地址低8位 (参照 SPI-DMA.c:167)
    DMA_UR1R_RXAH = (uint8_t)((uint16_t)rx_buf >> 8);  // 缓冲区地址高8位 (参照 SPI-DMA.c:168)
    DMA_UR1R_CR = 0xa1;                 // 使能DMA接收 + 清FIFO + 触发 (参照 SPI-DMA.c:169)

    // !!!!!!!!!!!!! 重要: USART-SPI全双工, 接收时必须同时启动发送来产生时钟
    // 参照 SPI-DMA.c:172
    DMA_UR1T_STA = 0x00;
    DMA_UR1T_CFG = 0x80;
    DMA_UR1T_AMT = (uint8_t)len;
    DMA_UR1T_AMTH = (uint8_t)(len >> 8);
    DMA_UR1T_TXAL = (uint8_t)((uint16_t)rx_buf);       // TX也指向rx_buf (dummy写)
    DMA_UR1T_TXAH = (uint8_t)((uint16_t)rx_buf >> 8);
    DMA_UR1T_CR = 0x80;                 // 使能DMA发送 (产生SPI时钟)
    // !!!!!!!!!!!!!
}

/******************************************************************************
 * @brief    USART1-SPI DMA同时读写
 * @param    tx_buf  - 发送数据缓冲区指针 (xdata空间)
 * @param    rx_buf  - 接收数据缓冲区指针 (xdata空间)
 * @param    len     - 读写字节数
 * @return   无
 * @note     非阻塞, 通过回调通知完成
 ******************************************************************************/
void USART1_SPI_DMA_WriteReadBytes_IT(uint8_t xdata *tx_buf, uint8_t xdata *rx_buf, uint16_t len)
{
    while (usart1_spi_dma_busy);        // 等待上次传输完成

    usart1_spi_dma_busy = 1;

    len--;                              // DMA传输字节数 = 实际值 - 1

    // TX DMA
    DMA_UR1T_STA = 0x00;
    DMA_UR1T_CFG = 0x80;
    DMA_UR1T_AMT = (uint8_t)len;
    DMA_UR1T_AMTH = (uint8_t)(len >> 8);
    DMA_UR1T_TXAL = (uint8_t)((uint16_t)tx_buf);
    DMA_UR1T_TXAH = (uint8_t)((uint16_t)tx_buf >> 8);

    // RX DMA
    DMA_UR1R_STA = 0x00;
    DMA_UR1R_CFG = 0x80;
    DMA_UR1R_AMT = (uint8_t)len;
    DMA_UR1R_AMTH = (uint8_t)(len >> 8);
    DMA_UR1R_RXAL = (uint8_t)((uint16_t)rx_buf);
    DMA_UR1R_RXAH = (uint8_t)((uint16_t)rx_buf >> 8);

    // 启动 DMA (先RX, 再TX)
    DMA_UR1R_CR = 0xa1;                 // RX: 使能 + 清FIFO + 触发
    DMA_UR1T_CR = 0x80;                 // TX: 使能
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>
