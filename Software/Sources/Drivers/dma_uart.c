#include "dma_uart.h"
#include "dma_uart_ring.h"
/* 定义缓冲区大小 */
#define UART_DMATxBufferLength 512
#define UART_DMARxBufferLength 512
#define UART_TxRingBufferLength 2048
#define UART_RxRingBufferLength 2048

/* 物理与逻辑缓冲区 (必须在 XDATA 或 EDATA) */
static uint8_t xdata uart5_dma_tx[UART_DMATxBufferLength];
static uint8_t xdata uart5_dma_rx[UART_DMARxBufferLength];
static uint8_t xdata uart5_ring_tx[UART_TxRingBufferLength];
static uint8_t xdata uart5_ring_rx[UART_RxRingBufferLength];

// DMA 状态标志
static volatile uint8_t isDmaTxBusy = 0;

#define Baudrate 115200

dma_uart_manager_t uart5_mgr;
// ==========================================
// 内部函数声明
// ==========================================
void UART5_Timer2_Config(uint16_t dat);


/* --- 硬件控制回调函数 --- */
static void UART5_HW_StartTx(uint16_t len) {
    DMA_UR5T_TXAH = (uint8_t)((uint16_t)&uart5_dma_tx >> 8);
    DMA_UR5T_TXAL = (uint8_t)((uint16_t)&uart5_dma_tx);
    DMA_UR5T_AMT = (uint8_t)(len - 1);
    DMA_UR5T_AMTH = (uint8_t)((len - 1) >> 8);
    
    DMA_UR5T_STA = 0x00;
    DMA_UR5T_CR = 0xc0; /* 启动 DMA Tx */
}

static void UART5_HW_RestartRx(void) {
    DMA_UR5R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR5R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
    DMA_UR5R_CR = 0xa1; /* 重新启动 DMA Rx */
}

/* --- OS 包装回调函数 --- */
static void OS_EnterCritical(void) { taskENTER_CRITICAL(); }
static void OS_ExitCritical(void)  { taskEXIT_CRITICAL(); }
static void OS_Delay(uint32_t ms)  { vTaskDelay(pdMS_TO_TICKS(ms)); }
// ==========================================
// 硬件配置函数
// ==========================================

void UART5_Timer2_Config(uint16_t dat) {
    T2R = 0;    // Timer stop
    T2_CT = 0;  // Timer2 set As Timer
    T2x12 = 1;  // Timer2 set as 1T mode
    T2H = dat / 256;
    T2L = dat % 256;
    ET2 = 0;    // 禁止中断
    T2R = 1;    // Timer run enable
}

void UART5_Config(uint8_t brt) {
    if(brt == 2) {
        S5CFG &= ~0x01; // 串口5选择定时器2为波特率发生器
        UART5_Timer2_Config((uint16_t)(65536UL - (MAIN_Fosc / 4) / Baudrate));
        S5CON = 0x50;   // 8位数据, 允许接收
    } else {
        S5CFG |= 0x01;  //串口5选择定时器5为波特率发生器
        S5CON = 0x50;   //8位数据, 允许接收
        TH5 = (65536UL - (MAIN_Fosc / 4) / Baudrate) / 256;
        TL5 = (65536UL - (MAIN_Fosc / 4) / Baudrate) % 256;
        T6T5M &= ~0x0f;
        T6T5M |= 0x0a;
    }

    UART5_SwitchP0405();

   
    UR5TOCR = 0xc0;     //bit7:使能超时接收，bit6:使能超时中断，bit5:超时时钟选择 1:系统时钟  0:串口数据位率(波特率)
    UR5TOTL = 0xA0;
    UR5TOTH = 0x00;
    UR5TOTE = 0x00;     //需要写 UR5TOTE 后，新的TO值才会生效
}

void UART5_DMA_Config(void) {
    // TX DMA 配置 (初始不启动)
    DMA_UR5T_CFG = 0x80;        // Enable Interrupt
    DMA_UR5T_STA = 0x00;
    
    // RX DMA 配置
    DMA_UR5R_CFG = 0x80;        // Enable Interrupt
    DMA_UR5R_STA = 0x00;
    
    DMA_UART5_SetTxIntPriority(1);      //设置中断为较低优先级
    DMA_UART5_SetRxIntPriority(1);      //设置中断为较低优先级
    // 设置 RX DMA 指向 buffer
    DMA_UR5R_RXAH = (uint8_t)((uint16_t)&uart5_dma_rx >> 8);
    DMA_UR5R_RXAL = (uint8_t)((uint16_t)&uart5_dma_rx);
    
    // 设置最大接收长度 (n+1)
    DMA_UR5R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR5R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);

    // 启动 RX DMA
    DMA_UR5R_CR = 0xa1; // Enable DMA, Start Rx, Clear FIFO
}

void UART5_Init(void) {
    UART5_Config(2);
    UART5_DMA_Config();

    /* 组装管理器 */
    uart5_mgr.dma_tx_buf = uart5_dma_tx;
    uart5_mgr.dma_tx_buf_size = UART_DMATxBufferLength;
    uart5_mgr.dma_rx_buf = uart5_dma_rx;
    uart5_mgr.dma_rx_buf_size = UART_DMARxBufferLength;
    uart5_mgr.ring_tx_buf = uart5_ring_tx;
    uart5_mgr.ring_tx_buf_size = UART_TxRingBufferLength;
    uart5_mgr.ring_rx_buf = uart5_ring_rx;
    uart5_mgr.ring_rx_buf_size = UART_RxRingBufferLength;

    uart5_mgr.hw_start_dma_tx_cb = UART5_HW_StartTx;
    uart5_mgr.hw_restart_dma_rx_cb = UART5_HW_RestartRx;
    uart5_mgr.os_enter_critical_cb = OS_EnterCritical;
    uart5_mgr.os_exit_critical_cb = OS_ExitCritical;
    uart5_mgr.os_delay_cb = OS_Delay;

    DMA_UART_Manager_Init(&uart5_mgr);
}


// ==========================================
// 中断服务函数
// ==========================================

// UART5 中断 (处理超时接收)
void UART5_ISR(void) interrupt UART5_VECTOR {
    uint16_t receivedCount;
//    uint16_t remain;
    // 发送中断通常由DMA处理，这里主要处理接收超时
    if (UART5_CheckTxFlag()) {
        UART5_ClearTxFlag();
    }
    if (UART5_CheckRxFlag()) {
        UART5_ClearRxFlag();
    }

    // 接收超时：表示一帧数据接收完毕，但未填满 DMA 缓冲区
    if (UART5_CheckTimeoutFlag()) {
        UR5TOSR = 0x80; 
        S5CON &= ~0x02;

       
        receivedCount = ((uint16_t)DMA_UR5R_DONEH << 8) + DMA_UR5R_DONE; 
         DMA_UR5R_CR = 0; /* 停止 DMA 读取状态 */
        /* 仅需将事件推给管理器 */
        DMA_UART_OnRxTimeout_ISR(&uart5_mgr, receivedCount);
    }
}

// DMA 发送中断
void UART5_DMATx_ISR(void) interrupt DMA_UR5T_VECTOR {
    if (DMA_UART5_CheckTxFlag()) {
        DMA_UART5_ClearTxFlag();
        // 当前一批数据发送完毕，检查 RingBuffer 是否还有数据
        // 如果有，继续发送下一批
        DMA_UART_OnTxDone_ISR(&uart5_mgr);
    }
    if (DMA_UR5T_STA & 0x04)    //数据覆盖
    {
        DMA_UR5T_STA &= ~0x04;
    }
}

// DMA 接收中断 (缓冲区满)
void UART5_DMARx_ISR(void) interrupt DMA_UR5R_VECTOR {
    if (DMA_UR5R_STA & 0x01) {
        DMA_UR5R_STA &= ~0x01;
        
        // DMA 缓冲区已满，全部拷走
        DMA_UART_OnRxFull_ISR(&uart5_mgr);

        // 重置并重新启动 DMA
        DMA_UR5R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
        DMA_UR5R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
        DMA_UR5R_CR = 0xa1; // Restart
    }
    if (DMA_UR5R_STA & 0x02)    //数据丢弃
    {
        DMA_UR5R_STA &= ~0x02;
    }
}