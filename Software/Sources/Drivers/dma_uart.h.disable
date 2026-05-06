#include "dma_uart.h"
#include "dma_uart_ring.h"

/* 定义缓冲区大小 */
#define UART_DMATxBufferLength 512
#define UART_DMARxBufferLength 512
#define UART_TxRingBufferLength 2048
#define UART_RxRingBufferLength 2048

/* 物理与逻辑缓冲区 (必须在 XDATA 或 EDATA) */
static uint8_t xdata uart4_dma_tx[UART_DMATxBufferLength] = {0};
static uint8_t xdata uart4_dma_rx[UART_DMARxBufferLength] = {0};
static uint8_t xdata uart4_ring_tx[UART_TxRingBufferLength] = {0};
static uint8_t xdata uart4_ring_rx[UART_RxRingBufferLength] = {0};

// DMA 状态标志
static volatile uint8_t isDmaTxBusy = 0;

#define Baudrate 115200

dma_uart_manager_t uart4_mgr = {0};

// ==========================================
// 内部函数声明
// ==========================================
void UART4_Timer2_Config(uint16_t dat);


/* --- 硬件控制回调函数 --- */
static void UART4_HW_StartTx(uint16_t len) {
    DMA_UR4T_TXAH = (uint8_t)((uint16_t)&uart4_dma_tx >> 8);
    DMA_UR4T_TXAL = (uint8_t)((uint16_t)&uart4_dma_tx);
    DMA_UR4T_AMT = (uint8_t)(len - 1);
    DMA_UR4T_AMTH = (uint8_t)((len - 1) >> 8);
    
    DMA_UR4T_STA = 0x00;
    DMA_UR4T_CR = 0xc0; /* 启动 DMA Tx: EN=1, TRIG=1 */
}

static void UART4_HW_RestartRx(void) {
    DMA_UR4R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR4R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
    DMA_UR4R_CR = 0xa1; /* 重新启动 DMA Rx: EN=1, ClearFIFO=1 */
}

/* --- OS 包装回调函数 --- */
static void OS_EnterCritical(void) { taskENTER_CRITICAL(); }
static void OS_ExitCritical(void)  { taskEXIT_CRITICAL(); }
static void OS_Delay(uint32_t ms)  { vTaskDelay(pdMS_TO_TICKS(ms)); }

// ==========================================
// 硬件配置函数
// ==========================================

void UART4_Timer2_Config(uint16_t dat) {
    T2R = 0;    // Timer stop
    T2_CT = 0;  // Timer2 set As Timer
    T2x12 = 1;  // Timer2 set as 1T mode
    T2H = dat / 256;
    T2L = dat % 256;
    ET2 = 0;    // 禁止中断
    T2R = 1;    // Timer run enable
}

void UART4_Config(uint8_t brt) {
    if(brt == 2) {
        S4CFG &= ~0x01; // 串口4选择定时器2为波特率发生器
        UART4_Timer2_Config((uint16_t)(65536UL - (MAIN_Fosc / 4) / Baudrate));
        S4CON = 0x50;   // 8位数据, 允许接收
    } else {
        S4CFG |= 0x01;  // 串口4选择定时器4为波特率发生器
        S4CON = 0x50;   // 8位数据, 允许接收
        // 定时器4配置逻辑参照原代码中对定时器5的处理
        T4H = (65536UL - (MAIN_Fosc / 4) / Baudrate) / 256;
        T4L = (65536UL - (MAIN_Fosc / 4) / Baudrate) % 256;
        T3T4M &= ~0xf0; // 清除 T4 相关位
        T3T4M |= 0xa0;  // 1T 模式, 开始运行
    }

    UART4_SwitchP5253(); // 切换至 P5.2 RXD4, P5.3 TXD4

    // 串口接收超时配置
    UR4TOCR = 0xc0;     // bit7:使能超时接收, bit6:使能超时中断, bit5:时钟源选择
    UR4TOTL = 0xA0;
    UR4TOTH = 0x00;
    UR4TOTE = 0x00;     // 触发生效

    ES4 = 1;
}

void UART4_DMA_Config(void) {
    // TX DMA 配置 (初始不启动)
    DMA_UR4T_CFG = 0x80;        // Enable Interrupt
    DMA_UR4T_STA = 0x00;
    
    // RX DMA 配置
    DMA_UR4R_CFG = 0x80;        // Enable Interrupt
    DMA_UR4R_STA = 0x00;
    
    DMA_UART4_SetTxIntPriority(1);      // 设置中断为较低优先级
    DMA_UART4_SetRxIntPriority(1);      // 设置中断为较低优先级
    
    // 设置 RX DMA 指向 buffer
    DMA_UR4R_RXAH = (uint8_t)((uint16_t)&uart4_dma_rx >> 8);
    DMA_UR4R_RXAL = (uint8_t)((uint16_t)&uart4_dma_rx);
    
    // 设置最大接收长度 (n+1)
    DMA_UR4R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR4R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);

    // 启动 RX DMA
    DMA_UR4R_CR = 0xa1; // Enable DMA, Start Rx, Clear FIFO
}

void UART4_Init(void) {
    UART4_Config(2); // 默认使用 Timer2
    UART4_DMA_Config();
    

    /* 组装管理器 */
    uart4_mgr.dma_tx_buf = uart4_dma_tx;
    uart4_mgr.dma_tx_buf_size = UART_DMATxBufferLength;
    uart4_mgr.dma_rx_buf = uart4_dma_rx;
    uart4_mgr.dma_rx_buf_size = UART_DMARxBufferLength;
    uart4_mgr.ring_tx_buf = uart4_ring_tx;
    uart4_mgr.ring_tx_buf_size = UART_TxRingBufferLength;
    uart4_mgr.ring_rx_buf = uart4_ring_rx;
    uart4_mgr.ring_rx_buf_size = UART_RxRingBufferLength;

    uart4_mgr.hw_start_dma_tx_cb = UART4_HW_StartTx;
    uart4_mgr.hw_restart_dma_rx_cb = UART4_HW_RestartRx;
    uart4_mgr.os_enter_critical_cb = OS_EnterCritical;
    uart4_mgr.os_exit_critical_cb = OS_ExitCritical;
    uart4_mgr.os_delay_cb = OS_Delay;

    DMA_UART_Manager_Init(&uart4_mgr);
}


// ==========================================
// 中断服务函数
// ==========================================

// UART4 中断 (处理超时接收)
void UART4_ISR(void) interrupt UART4_VECTOR {
    uint16_t receivedCount;
    
    if (UART4_CheckTxFlag()) {
        UART4_ClearTxFlag();
    }
    if (UART4_CheckRxFlag()) {
        UART4_ClearRxFlag();
    }

    // 接收超时：表示一帧数据接收完毕，但未填满 DMA 缓冲区
    if (UART4_CheckTimeoutFlag()) {
        UR4TOSR = 0x80; // 清除超时标志
        S4CON &= ~0x01; // 清除串口接收中断请求

        // 读取已完成接收的字节数
        receivedCount = ((uint16_t)DMA_UR4R_DONEH << 8) + DMA_UR4R_DONE; 
        DMA_UR4R_CR = 0; /* 停止 DMA 读取状态，确保计数器不抖动 */
        
        /* 调用管理器逻辑处理超时数据 */
        DMA_UART_OnRxTimeout_ISR(&uart4_mgr, receivedCount);
    }
}

// DMA 发送中断
void UART4_DMATx_ISR(void) interrupt DMA_UR4T_VECTOR {
    if (DMA_UART4_CheckTxFlag()) {
        DMA_UART4_ClearTxFlag();
        // 当前一批数据发送完毕，通知管理器检查环形缓冲区
        DMA_UART_OnTxDone_ISR(&uart4_mgr);
    }
    if (DMA_UR4T_STA & 0x04) { // 数据覆盖标志
        DMA_UR4T_STA &= ~0x04;
    }
}

// DMA 接收中断 (缓冲区满)
void UART4_DMARx_ISR(void) interrupt DMA_UR4R_VECTOR {
    if (DMA_UR4R_STA & 0x01) { // 传输完成标志 (AMT 满)
        DMA_UR4R_STA &= ~0x01;
        
        // DMA 缓冲区已满，全部搬运至环形缓冲区
        DMA_UART_OnRxFull_ISR(&uart4_mgr);

        // 重置长度并重新启动 DMA RX
        DMA_UR4R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
        DMA_UR4R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
        DMA_UR4R_CR = 0xa1; 
    }
    if (DMA_UR4R_STA & 0x02) { // 数据丢弃标志
        DMA_UR4R_STA &= ~0x02;
    }
}