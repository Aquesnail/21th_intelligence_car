#include "dma_uart.h"

// ==========================================
// 变量定义
// ==========================================

// 物理 DMA 缓冲区 (必须在 XDATA 或 EDATA，STC32默认变量在EDATA/XDATA)
static uint8_t xdata uartDMATxBuffer[UART_DMATxBufferLength];
static uint8_t xdata uartDMARxBuffer[UART_DMARxBufferLength];

// 逻辑 环形缓冲区
static uint8_t xdata uartTxRingBuffer[UART_TxRingBufferLength];
static uint8_t xdata uartRxRingBuffer[UART_RxRingBufferLength];

// 环形缓冲区指针
static volatile uint16_t txHead = 0;
static volatile uint16_t txTail = 0;
static volatile uint16_t txCount = 0;

static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;
static volatile uint16_t rxCount = 0;

// DMA 状态标志
static volatile uint8_t isDmaTxBusy = 0;

#define Baudrate 115200

// ==========================================
// 内部函数声明
// ==========================================
static void Start_DMA_Tx_From_RingBuffer(void);
static void Copy_DMA_Rx_To_RingBuffer(uint16_t length);
void UART5_Timer2_Config(uint16_t dat);

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
    
    // 设置 RX DMA 指向 buffer
    DMA_UR5R_RXAH = (uint8_t)((uint16_t)&uartDMARxBuffer >> 8);
    DMA_UR5R_RXAL = (uint8_t)((uint16_t)&uartDMARxBuffer);
    
    // 设置最大接收长度 (n+1)
    DMA_UR5R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
    DMA_UR5R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);

    // 启动 RX DMA
    DMA_UR5R_CR = 0xa1; // Enable DMA, Start Rx, Clear FIFO
}

void UART5_Init(void) {
    UART5_Config(2);
    UART5_DMA_Config();
}

// ==========================================
// 内部辅助逻辑
// ==========================================

// 尝试从 RingBuffer 取数据填入 DMA 并发送
static void Start_DMA_Tx_From_RingBuffer(void) {
    uint16_t bytesToSend;
    uint16_t i;
    
    // 如果 RingBuffer 为空，标记发送空闲并退出
    if (txCount == 0) {
        isDmaTxBusy = 0;
        return;
    }

    isDmaTxBusy = 1;

    // 计算本次能从 RingBuffer 取出的最大连续长度
    // 注意：如果是回绕（Tail < Head），只能取到缓冲区末尾
    if (txTail >= txHead) {
        bytesToSend = txTail - txHead;
    } else {
        bytesToSend = UART_TxRingBufferLength - txHead;
    }

    // 限制单次 DMA 传输长度不超过 DMA 物理缓冲区大小
    if (bytesToSend > UART_DMATxBufferLength) {
        bytesToSend = UART_DMATxBufferLength;
    }

    // 搬运数据到 DMA 专用缓冲区
    for (i = 0; i < bytesToSend; i++) {
        uartDMATxBuffer[i] = uartTxRingBuffer[txHead];
        txHead = (txHead + 1) % UART_TxRingBufferLength;
    }

    // FreeRTOS 临界区：保护 txCount 更新
    taskENTER_CRITICAL();
    txCount -= bytesToSend;
    taskEXIT_CRITICAL();

    // 配置 DMA 发送寄存器
    DMA_UR5T_TXAH = (uint8_t)((uint16_t)&uartDMATxBuffer >> 8);
    DMA_UR5T_TXAL = (uint8_t)((uint16_t)&uartDMATxBuffer);
    
    // 设置长度 (寄存器值为 len - 1)
    DMA_UR5T_AMT = (uint8_t)(bytesToSend - 1);
    DMA_UR5T_AMTH = (uint8_t)((bytesToSend - 1) >> 8);

    // 启动 DMA 发送
    DMA_UR5T_CR = 0xc0; // Enable DMA, Start Tx
}

// 将接收到的 DMA 数据拷入 RingBuffer
static void Copy_DMA_Rx_To_RingBuffer(uint16_t length) {
    uint16_t i;
    uint16_t freeSpace;

    if (length == 0) return;

    // 简单的溢出保护：如果 RingBuffer 满，则丢弃新数据
    // 实际项目中可能需要更复杂的错误处理
    freeSpace = UART_RxRingBufferLength - rxCount;
    if (length > freeSpace) {
        length = freeSpace; 
    }

    for (i = 0; i < length; i++) {
        uartRxRingBuffer[rxTail] = uartDMARxBuffer[i];
        rxTail = (rxTail + 1) % UART_RxRingBufferLength;
    }

    // 临界区保护 rxCount
   // taskENTER_CRITICAL();
    rxCount += length;
    //taskEXIT_CRITICAL();
}

// ==========================================
// 外部接口函数 (Read / Write)
// ==========================================

// 将数据写入发送缓冲区，如果 DMA 空闲则自动启动
uint16_t UART5_Write(char* dat, uint16_t len) {
    uint16_t i;
    uint16_t freeSpace;
    

    if(len > UART_TxRingBufferLength) len = UART_TxRingBufferLength; //超出缓冲长度直接截断
    // 等待缓冲区有足够空间 (简单的阻塞模式，也可改为非阻塞返回写入数量)
    do {
       // taskENTER_CRITICAL();
        freeSpace = UART_TxRingBufferLength - txCount;
       // taskEXIT_CRITICAL();
        // 如果是 RTOS，这里可以加个 vTaskDelay(1) 避免死等占满 CPU

        if(freeSpace < len){
            vTaskDelay(pdMS_TO_TICKS(1));
        }

    } while (freeSpace < len);

    // 写入 RingBuffer
    for (i = 0; i < len; i++) {
        uartTxRingBuffer[txTail] = dat[i];
        txTail = (txTail + 1) % UART_TxRingBufferLength;
        
      //  taskENTER_CRITICAL();
        txCount++;
      //  taskEXIT_CRITICAL();
    }

    // 如果 DMA 当前没有在工作，手动触发第一次发送
    taskENTER_CRITICAL();
    if (isDmaTxBusy == 0) {
        Start_DMA_Tx_From_RingBuffer();
    }
    taskEXIT_CRITICAL();

    return len;
}

// 从接收缓冲区读取数据
uint16_t UART5_Read(char* dat, uint16_t len) {
    uint16_t i;
    uint16_t readLen = 0;

    taskENTER_CRITICAL();
    if (rxCount < len) {
        readLen = rxCount;
    } else {
        readLen = len;
    }
    taskEXIT_CRITICAL();

    for (i = 0; i < readLen; i++) {
        dat[i] = uartRxRingBuffer[rxHead];
        rxHead = (rxHead + 1) % UART_RxRingBufferLength;
    }

    taskENTER_CRITICAL();
    rxCount -= readLen;
    taskEXIT_CRITICAL();

    return readLen;
}

// 查询当前有多少字节可读
uint16_t UART5_Available(void) {
    uint16_t count;
    taskENTER_CRITICAL();
    count = rxCount;
    taskEXIT_CRITICAL();
    return count;
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

			 // 2. 计算已接收字节数
        receivedCount = ((u16)DMA_UR5R_DONEH << 8) + DMA_UR5R_DONE; 
        // 1. 停止 DMA 接收以读取状态
        DMA_UR5R_CR = 0 ; // Clear Start Bit (Stop)

       

        // 3. 搬运数据
        Copy_DMA_Rx_To_RingBuffer(receivedCount);

        // 4. 重置并重新启动 DMA 接收
        DMA_UR5R_AMT = (uint8_t)(UART_DMARxBufferLength - 1);
        DMA_UR5R_AMTH = (uint8_t)((UART_DMARxBufferLength - 1) >> 8);
        DMA_UR5R_CR = 0xa1; // Restart
    }
}

// DMA 发送中断
void UART5_DMATx_ISR(void) interrupt DMA_UR5T_VECTOR {
    if (DMA_UART5_CheckTxFlag()) {
        DMA_UART5_ClearTxFlag();
        // 当前一批数据发送完毕，检查 RingBuffer 是否还有数据
        // 如果有，继续发送下一批
        Start_DMA_Tx_From_RingBuffer();
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
        Copy_DMA_Rx_To_RingBuffer(UART_DMARxBufferLength);

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