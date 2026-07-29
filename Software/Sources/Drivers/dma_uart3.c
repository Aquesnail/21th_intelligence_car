#include "dma_uart3.h"
#include "dma_uart_ring.h"
#include <string.h>

/* 定义缓冲区大小 */
#define UART_DMATxBufferLength 512
#define UART_DMARxBufferLength 512
#define UART_TxRingBufferLength 1024*10
#define UART_RxRingBufferLength 1024*10

/* 物理与逻辑缓冲区 (必须在 XDATA 或 EDATA) */
static uint8_t xdata UART3_dma_tx[UART_DMATxBufferLength];   // 运行时 memset 初始化
static uint8_t xdata UART3_dma_rx[UART_DMARxBufferLength];   // 运行时 memset 初始化
static uint8_t xdata UART3_ring_tx[UART_TxRingBufferLength]; // 运行时 memset 初始化
static uint8_t xdata UART3_ring_rx[UART_RxRingBufferLength]; // 运行时 memset 初始化

// DMA 状态标志
static volatile uint8_t isDmaTxBusy = 0;

#define Baudrate 256000

dma_uart_manager_t uart3_mgr; // 运行时 memset 初始化

// ==========================================
// 内部函数声明
// ==========================================
void UART3_Timer2_Config(uint16_t dat);


/* --- 硬件控制回调函数 --- */
static void UART3_HW_StartTx(uint16_t len) {
    DMA_UR3T_TXAH = (uint8_t)((uint16_t)&UART3_dma_tx >> 8);
    DMA_UR3T_TXAL = (uint8_t)((uint16_t)&UART3_dma_tx);
    DMA_UR3T_AMT = (uint8_t)(len - 1);
    DMA_UR3T_AMTH = (uint8_t)((len - 1) >> 8);
    
    DMA_UR3T_STA = 0x00;
    DMA_UR3T_CR = 0xc0; /* 启动 DMA Tx: EN=1, TRIG=1 */
}

static void UART3_HW_RestartRx(void) {
    DMA_UR3R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR3R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
    DMA_UR3R_CR = 0xa1; /* 重新启动 DMA Rx: EN=1, ClearFIFO=1 */
}

/* --- OS 包装回调函数 --- */
static void OS_EnterCritical(void) { taskENTER_CRITICAL(); }
static void OS_ExitCritical(void)  { taskEXIT_CRITICAL(); }
static void OS_Delay(uint32_t ms)  { vTaskDelay(pdMS_TO_TICKS(ms)); }

// ==========================================
// 硬件配置函数
// ==========================================

void UART3_DMA_Config(void) {

//     UART3_TimeoutScale_BRT();           //设置串口波特率为接收超时时间基准
//     //UART3_SetTimeoutInterval(0x00000a); //设置接收超时时间长度
//     UART3_EnableTimeout();              //使能串口接收超时功能

//     DMA_UART3_ClearTxFlag();            //清除串口发送DMA中断标志
//     DMA_UART3_SetTxBusPriority(0);      //设置总线访问为最低优先级
//     DMA_UART3_SetTxIntPriority(0);      //设置中断为最低优先级
//     DMA_UART3_EnableTxInt();            //使能串口2DMA发送中断
//     DMA_UART3_EnableTx();               //使能串口2DMA发送功能
// //  DMA_UART3_TriggerTx();              //触发串口2DMA发送
//     DMA_UART3_ClearFIFO();              //清空串口DMA接收FIFO缓冲区
//     DMA_UART3_ClearRxFlag();            //清除串口接收DMA中断标志
//     DMA_UART3_SetRxBusPriority(0);      //设置总线访问为最低优先级
//     DMA_UART3_SetRxIntPriority(0);      //设置中断为最低优先级
//     DMA_UART3_EnableRxInt();            //使能串口2DMA接收中断
//     DMA_UART3_EnableRx();               //使能串口2DMA接收功能
// //  DMA_UART3_TriggerRx();              //触发串口2DMA接收

//     UART3_SetTxFlag();                  //设置发送标志以提高发送效率
//     // // TX DMA 配置 (初始不启动)
    DMA_UR3T_CFG = 0x80;        // Enable Interrupt
    DMA_UR3T_STA = 0x00;
    
    // RX DMA 配置
    DMA_UR3R_CFG = 0x80;        // Enable Interrupt
    DMA_UR3R_STA = 0x00;
    
    DMA_UART3_SetTxIntPriority(1);      // 设置中断为较低优先级
    DMA_UART3_SetRxIntPriority(1);      // 设置中断为较低优先级
    
    // 设置 RX DMA 指向 buffer
    DMA_UR3R_RXAH = (uint8_t)((uint16_t)&UART3_dma_rx >> 8);
    DMA_UR3R_RXAL = (uint8_t)((uint16_t)&UART3_dma_rx);
    
    // 设置最大接收长度 (n+1)
    DMA_UR3R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR3R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);

    // 启动 RX DMA
    DMA_UR3R_CR = 0xa1; // Enable DMA, Start Rx, Clear FIFO

}
static void SetTimer2Baudraye(u16 dat)  // 选择波特率, 2: 使用Timer2做波特率, 其它值: 使用Timer1做波特率.
{
    T2R = 0;    //Timer stop
    T2_CT = 0;  //Timer2 set As Timer
    T2x12 = 1;  //Timer2 set as 1T mode
    T2H = dat / 256;
    T2L = dat % 256;
    ET2 = 0;    //禁止中断
    T2R = 1;    //Timer run enable
}

void UART3_Init(void) {
    /* 运行时初始化大数组/结构体，避免 Flash 镜像占用 */
    memset(UART3_dma_tx,  0, sizeof(UART3_dma_tx));
    memset(UART3_dma_rx,  0, sizeof(UART3_dma_rx));
    memset(UART3_ring_tx, 0, sizeof(UART3_ring_tx));
    memset(UART3_ring_rx, 0, sizeof(UART3_ring_rx));
    memset(&uart3_mgr,    0, sizeof(uart3_mgr));

    #ifdef BAUDRATE
#undef BAUDRATE
#endif
#define BAUDRATE                (256000)
#define T2_RELOAD               (65536 - (SYSCLK / BAUDRATE + 2) / 4)

    UART3_SwitchP0001();                //设置串口数据端口: RxD3 (P0.0), TxD3 (P0.1)

    SetTimer2Baudraye((u16)T2_RELOAD);

        S3CON = (S3CON & 0x3f) | 0x40;  //UART3模式, 0x00: 同步移位输出, 0x40: 8位数据,可变波特率, 0x80: 9位数据,固定波特率, 0xc0: 9位数据,可变波特率
//        ES3   = 1;       //允许中断
        S3REN = 1;         //允许接收
       // UART3_SW(UART3_SW_P10_P11);     //UART3_SW_P10_P11,UART3_SW_P46_P47,UART3_SW_P84_P85,UART3_SW_PA0_PA1

        UR3TOCR = 0xc0; //bit7:使能超时接收，bit6:使能超时中断，bit5:超时时钟选择 1:系统时钟  0:串口数据位率(波特率)
        UR3TOTL = 0x20;
        UR3TOTH = 0x00;
        UR3TOTE = 0x00; //需要写 UR3TOTE 后，新的TO值才会生效
    UART3_DMA_Config();
    

    /* 组装管理器 */
    uart3_mgr.dma_tx_buf = UART3_dma_tx;
    uart3_mgr.dma_tx_buf_size = UART_DMATxBufferLength;
    uart3_mgr.dma_rx_buf = UART3_dma_rx;
    uart3_mgr.dma_rx_buf_size = UART_DMARxBufferLength;
    uart3_mgr.ring_tx_buf = UART3_ring_tx;
    uart3_mgr.ring_tx_buf_size = UART_TxRingBufferLength;
    uart3_mgr.ring_rx_buf = UART3_ring_rx;
    uart3_mgr.ring_rx_buf_size = UART_RxRingBufferLength;

    uart3_mgr.hw_start_dma_tx_cb = UART3_HW_StartTx;
    uart3_mgr.hw_restart_dma_rx_cb = UART3_HW_RestartRx;
    uart3_mgr.os_enter_critical_cb = OS_EnterCritical;
    uart3_mgr.os_exit_critical_cb = OS_ExitCritical;
    uart3_mgr.os_delay_cb = OS_Delay;

    DMA_UART_Manager_Init(&uart3_mgr);
}


// ==========================================
// 中断服务函数
// ==========================================

// UART3 中断 (处理超时接收)
void UART3_ISR(void) interrupt UART3_VECTOR {
    uint16_t receivedCount;
    
    if (UART3_CheckTxFlag()) {
        UART3_ClearTxFlag();
    }
    if (UART3_CheckRxFlag()) {
        UART3_ClearRxFlag();
    }

    // 接收超时：表示一帧数据接收完毕，但未填满 DMA 缓冲区
    if (UART3_CheckTimeoutFlag()) {
        UR3TOSR = 0x80; // 清除超时标志
        S4CON &= ~0x01; // 清除串口接收中断请求

        // 读取已完成接收的字节数
        receivedCount = ((uint16_t)DMA_UR3R_DONEH << 8) + DMA_UR3R_DONE; 
        DMA_UR3R_CR = 0; /* 停止 DMA 读取状态，确保计数器不抖动 */
        
        /* 调用管理器逻辑处理超时数据 */
        DMA_UART_OnRxTimeout_ISR(&uart3_mgr, receivedCount);
    }
}

// DMA 发送中断
void UART3_DMATx_ISR(void) interrupt DMA_UR3T_VECTOR {
    if (DMA_UART3_CheckTxFlag()) {
        DMA_UART3_ClearTxFlag();
        // 当前一批数据发送完毕，通知管理器检查环形缓冲区
        DMA_UART_OnTxDone_ISR(&uart3_mgr);
    }
    if (DMA_UR3T_STA & 0x04) { // 数据覆盖标志
        DMA_UR3T_STA &= ~0x04;
    }
}

// DMA 接收中断 (缓冲区满)
void UART3_DMARx_ISR(void) interrupt DMA_UR3R_VECTOR {
    if (DMA_UR3R_STA & 0x01) { // 传输完成标志 (AMT 满)
        DMA_UR3R_STA &= ~0x01;
        
        // DMA 缓冲区已满，全部搬运至环形缓冲区
        DMA_UART_OnRxFull_ISR(&uart3_mgr);

        // 重置长度并重新启动 DMA RX
        DMA_UR3R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
        DMA_UR3R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
        DMA_UR3R_CR = 0xa1; 
    }
    if (DMA_UR3R_STA & 0x02) { // 数据丢弃标志
        DMA_UR3R_STA &= ~0x02;
    }
}