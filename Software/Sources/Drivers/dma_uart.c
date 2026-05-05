#include "dma_uart.h"
#include "dma_uart_ring.h"

/* 定义缓冲区大小 */
#define UART_DMATxBufferLength 512
#define UART_DMARxBufferLength 512
#define UART_TxRingBufferLength 2048
#define UART_RxRingBufferLength 2048

/* 物理与逻辑缓冲区 (必须在 XDATA 或 EDATA) */
static uint8_t xdata uart2_dma_tx[UART_DMATxBufferLength] = {0};
static uint8_t xdata uart2_dma_rx[UART_DMARxBufferLength] = {0};
static uint8_t xdata uart2_ring_tx[UART_TxRingBufferLength] = {0};
static uint8_t xdata uart2_ring_rx[UART_RxRingBufferLength] = {0};

// DMA 状态标志
static volatile uint8_t isDmaTxBusy = 0;

#define Baudrate 115200

dma_uart_manager_t uart2_mgr = {0};

// ==========================================
// 内部函数声明
// ==========================================
void UART2_Timer2_Config(uint16_t dat);


/* --- 硬件控制回调函数 --- */
static void UART2_HW_StartTx(uint16_t len) {
    DMA_UR2T_TXAH = (uint8_t)((uint16_t)&uart2_dma_tx >> 8);
    DMA_UR2T_TXAL = (uint8_t)((uint16_t)&uart2_dma_tx);
    DMA_UR2T_AMT = (uint8_t)(len - 1);
    DMA_UR2T_AMTH = (uint8_t)((len - 1) >> 8);
    
    DMA_UR2T_STA = 0x00;
    DMA_UR2T_CR = 0xc0; /* 启动 DMA Tx: EN=1, TRIG=1 */
}

static void UART2_HW_RestartRx(void) {
    DMA_UR2R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR2R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
    DMA_UR2R_CR = 0xa1; /* 重新启动 DMA Rx: EN=1, ClearFIFO=1 */
}

/* --- OS 包装回调函数 --- */
static void OS_EnterCritical(void) { taskENTER_CRITICAL(); }
static void OS_ExitCritical(void)  { taskEXIT_CRITICAL(); }
static void OS_Delay(uint32_t ms)  { vTaskDelay(pdMS_TO_TICKS(ms)); }

// ==========================================
// 硬件配置函数
// ==========================================

void UART2_Timer2_Config(uint16_t dat) {
    T2R = 0;    // Timer stop
    T2_CT = 0;  // Timer2 set As Timer
    T2x12 = 1;  // Timer2 set as 1T mode
    T2H = dat / 256;
    T2L = dat % 256;
    ET2 = 0;    // 禁止中断
    T2R = 1;    // Timer run enable
}

void UART2_Config(uint8_t brt) {
    if(brt == 2) {
        S2CFG &= ~0x01; // 串口2选择定时器2为波特率发生器
        UART2_Timer2_Config((uint16_t)(65536UL - (MAIN_Fosc / 4) / Baudrate));
        S4CON = 0x50;   // 8位数据, 允许接收
    } else {
        S2CFG |= 0x01;  // 串口2选择定时器4为波特率发生器
        S2CON = 0x50;   // 8位数据, 允许接收
        // 定时器4配置逻辑参照原代码中对定时器5的处理
        T4H = (65536UL - (MAIN_Fosc / 4) / Baudrate) / 256;
        T4L = (65536UL - (MAIN_Fosc / 4) / Baudrate) % 256;
        T3T4M &= ~0xf0; // 清除 T4 相关位
        T3T4M |= 0xa0;  // 1T 模式, 开始运行
    }

    UART2_SwitchP1011(); // 切换至 P5.2 RXD4, P5.3 TXD4

    // 串口接收超时配置
    UR2TOCR = 0xc0;     // bit7:使能超时接收, bit6:使能超时中断, bit5:时钟源选择
    UR2TOTL = 0xA0;
    UR2TOTH = 0x00;
    UR2TOTE = 0x00;     // 触发生效

    ES4 = 1;
}

void UART2_DMA_Config(void) {
    // TX DMA 配置 (初始不启动)
    DMA_UR2T_CFG = 0x80;        // Enable Interrupt
    DMA_UR2T_STA = 0x00;
    
    // RX DMA 配置
    DMA_UR2R_CFG = 0x80;        // Enable Interrupt
    DMA_UR2R_STA = 0x00;
    
    DMA_UART2_SetTxIntPriority(1);      // 设置中断为较低优先级
    DMA_UART2_SetRxIntPriority(1);      // 设置中断为较低优先级
    
    // 设置 RX DMA 指向 buffer
    DMA_UR2R_RXAH = (uint8_t)((uint16_t)&uart2_dma_rx >> 8);
    DMA_UR2R_RXAL = (uint8_t)((uint16_t)&uart2_dma_rx);
    
    // 设置最大接收长度 (n+1)
    DMA_UR2R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR2R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);

    // 启动 RX DMA
    DMA_UR2R_CR = 0xa1; // Enable DMA, Start Rx, Clear FIFO
}

void UART2_Init(void) {
    UART2_Config(2); // 默认使用 Timer2
    UART2_DMA_Config();
    

    /* 组装管理器 */
    uart2_mgr.dma_tx_buf = uart2_dma_tx;
    uart2_mgr.dma_tx_buf_size = UART_DMATxBufferLength;
    uart2_mgr.dma_rx_buf = uart2_dma_rx;
    uart2_mgr.dma_rx_buf_size = UART_DMARxBufferLength;
    uart2_mgr.ring_tx_buf = uart2_ring_tx;
    uart2_mgr.ring_tx_buf_size = UART_TxRingBufferLength;
    uart2_mgr.ring_rx_buf = uart2_ring_rx;
    uart2_mgr.ring_rx_buf_size = UART_RxRingBufferLength;

    uart2_mgr.hw_start_dma_tx_cb = UART2_HW_StartTx;
    uart2_mgr.hw_restart_dma_rx_cb = UART2_HW_RestartRx;
    uart2_mgr.os_enter_critical_cb = OS_EnterCritical;
    uart2_mgr.os_exit_critical_cb = OS_ExitCritical;
    uart2_mgr.os_delay_cb = OS_Delay;

    DMA_UART_Manager_Init(&uart2_mgr);
}


// ==========================================
// 中断服务函数
// ==========================================

// UART2 中断 (处理超时接收)
void UART2_ISR(void) interrupt UART2_VECTOR {
    uint16_t receivedCount;
    
    if (UART2_CheckTxFlag()) {
        UART2_ClearTxFlag();
    }
    if (UART2_CheckRxFlag()) {
        UART2_ClearRxFlag();
    }

    // 接收超时：表示一帧数据接收完毕，但未填满 DMA 缓冲区
    if (UART2_CheckTimeoutFlag()) {
        UR2TOSR = 0x80; // 清除超时标志
        S4CON &= ~0x01; // 清除串口接收中断请求

        // 读取已完成接收的字节数
        receivedCount = ((uint16_t)DMA_UR2R_DONEH << 8) + DMA_UR2R_DONE; 
        DMA_UR2R_CR = 0; /* 停止 DMA 读取状态，确保计数器不抖动 */
        
        /* 调用管理器逻辑处理超时数据 */
        DMA_UART_OnRxTimeout_ISR(&uart2_mgr, receivedCount);
    }
}

// DMA 发送中断
void UART2_DMATx_ISR(void) interrupt DMA_UR2T_VECTOR {
    if (DMA_UART2_CheckTxFlag()) {
        DMA_UART2_ClearTxFlag();
        // 当前一批数据发送完毕，通知管理器检查环形缓冲区
        DMA_UART_OnTxDone_ISR(&uart2_mgr);
    }
    if (DMA_UR2T_STA & 0x04) { // 数据覆盖标志
        DMA_UR2T_STA &= ~0x04;
    }
}

// DMA 接收中断 (缓冲区满)
void UART2_DMARx_ISR(void) interrupt DMA_UR2R_VECTOR {
    if (DMA_UR2R_STA & 0x01) { // 传输完成标志 (AMT 满)
        DMA_UR2R_STA &= ~0x01;
        
        // DMA 缓冲区已满，全部搬运至环形缓冲区
        DMA_UART_OnRxFull_ISR(&uart2_mgr);

        // 重置长度并重新启动 DMA RX
        DMA_UR2R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
        DMA_UR2R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
        DMA_UR2R_CR = 0xa1; 
    }
    if (DMA_UR2R_STA & 0x02) { // 数据丢弃标志
        DMA_UR2R_STA &= ~0x02;
    }
}