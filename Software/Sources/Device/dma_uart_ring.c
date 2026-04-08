#include "dma_uart_ring.h"
#include <stdint.h>
#include <stddef.h>
/* ==========================================
 * 内部静态辅助函数声明
 * ========================================== */
static void _start_dma_tx_internal(dma_uart_manager_t* mgr);
static void _copy_dma_rx_to_ring(dma_uart_manager_t* mgr, uint16_t length);

/* ==========================================
 * 接口实现
 * ========================================== */

void DMA_UART_Manager_Init(dma_uart_manager_t* mgr) {
    if (mgr == NULL) return;

    mgr->tx_head = 0;
    mgr->tx_tail = 0;
    mgr->tx_count = 0;

    mgr->rx_head = 0;
    mgr->rx_tail = 0;
    mgr->rx_count = 0;

    mgr->is_dma_tx_busy = 0;
}

static void _start_dma_tx_internal(dma_uart_manager_t* mgr) {
    uint16_t bytesToSend;
    uint16_t i;
    
    /* 如果 RingBuffer 为空，标记发送空闲并退出 */
    if (mgr->tx_count == 0) {
        mgr->is_dma_tx_busy = 0;
        return;
    }

    mgr->is_dma_tx_busy = 1;

    /* 计算本次能从 RingBuffer 取出的最大连续长度 */
    if (mgr->tx_tail > mgr->tx_head) {
        bytesToSend = mgr->tx_tail - mgr->tx_head;
    } else {
        bytesToSend = mgr->ring_tx_buf_size - mgr->tx_head;
    }

    /* 限制单次 DMA 传输长度不超过 DMA 物理缓冲区大小 */
    if (bytesToSend > mgr->dma_tx_buf_size) {
        bytesToSend = mgr->dma_tx_buf_size;
    }

    /* 搬运数据到 DMA 专用缓冲区 */
    for (i = 0; i < bytesToSend; i++) {
        mgr->dma_tx_buf[i] = mgr->ring_tx_buf[mgr->tx_head];
        mgr->tx_head = (mgr->tx_head + 1) % mgr->ring_tx_buf_size;
    }

    mgr->tx_count -= bytesToSend;

    /* 调用硬件回调启动 DMA */
    if (mgr->hw_start_dma_tx_cb != NULL) {
        mgr->hw_start_dma_tx_cb(bytesToSend);
    }
}

static void _copy_dma_rx_to_ring(dma_uart_manager_t* mgr, uint16_t length) {
    uint16_t i;
    uint16_t freeSpace;

    if (length == 0 || mgr == NULL) return;

    freeSpace = mgr->ring_rx_buf_size - mgr->rx_count;
    if (length > freeSpace) {
        length = freeSpace; 
    }

    for (i = 0; i < length; i++) {
        mgr->ring_rx_buf[mgr->rx_tail] = mgr->dma_rx_buf[i];
        mgr->rx_tail = (mgr->rx_tail + 1) % mgr->ring_rx_buf_size;
    }

    mgr->rx_count += length;
}

uint16_t DMA_UART_Write(dma_uart_manager_t* mgr, const uint8_t* dat, uint16_t len) {
    uint16_t i;
    uint16_t freeSpace;
    
    if (mgr == NULL || len == 0) return 0;
    if (len > mgr->ring_tx_buf_size) len = mgr->ring_tx_buf_size;

    /* 1. 等待缓冲区有足够空间 */
    do {
        if (mgr->os_enter_critical_cb) mgr->os_enter_critical_cb();
        freeSpace = mgr->ring_tx_buf_size - mgr->tx_count;
        if (mgr->os_exit_critical_cb) mgr->os_exit_critical_cb();

        if (freeSpace < len) {
            if (mgr->os_delay_cb) mgr->os_delay_cb(1);
        }
    } while (freeSpace < len);

    /* 2. 核心写入操作 */
    if (mgr->os_enter_critical_cb) mgr->os_enter_critical_cb();

    for (i = 0; i < len; i++) {
        mgr->ring_tx_buf[mgr->tx_tail] = dat[i];
        mgr->tx_tail = (mgr->tx_tail + 1) % mgr->ring_tx_buf_size;
    }
    
    mgr->tx_count += len;

    /* 3. 如果 DMA 空闲，立即触发第一次发送 */
    if (mgr->is_dma_tx_busy == 0) {
        _start_dma_tx_internal(mgr);
    }

    if (mgr->os_exit_critical_cb) mgr->os_exit_critical_cb();

    return len;
}

uint16_t DMA_UART_Read(dma_uart_manager_t* mgr, uint8_t* dat, uint16_t len) {
    uint16_t i;
    uint16_t readLen = 0;

    if (mgr == NULL || dat == NULL) return 0;

    if (mgr->os_enter_critical_cb) mgr->os_enter_critical_cb();
    
    if (mgr->rx_count < len) {
        readLen = mgr->rx_count;
    } else {
        readLen = len;
    }

    for (i = 0; i < readLen; i++) {
        dat[i] = mgr->ring_rx_buf[mgr->rx_head];
        mgr->rx_head = (mgr->rx_head + 1) % mgr->ring_rx_buf_size;
    }

    mgr->rx_count -= readLen;
    
    if (mgr->os_exit_critical_cb) mgr->os_exit_critical_cb();

    return readLen;
}

uint16_t DMA_UART_Available(dma_uart_manager_t* mgr) {
    uint16_t count;
    if (mgr == NULL) return 0;

    if (mgr->os_enter_critical_cb) mgr->os_enter_critical_cb();
    count = mgr->rx_count;
    if (mgr->os_exit_critical_cb) mgr->os_exit_critical_cb();

    return count;
}

/* ==========================================
 * 中断服务接口 (供硬件ISR层调用)
 * ========================================== */

void DMA_UART_OnTxDone_ISR(dma_uart_manager_t* mgr) {
    if (mgr == NULL) return;
    _start_dma_tx_internal(mgr);
}

void DMA_UART_OnRxTimeout_ISR(dma_uart_manager_t* mgr, uint16_t received_len) {
    if (mgr == NULL) return;
    _copy_dma_rx_to_ring(mgr, received_len);
    if (mgr->hw_restart_dma_rx_cb) {
        mgr->hw_restart_dma_rx_cb();
    }
}

void DMA_UART_OnRxFull_ISR(dma_uart_manager_t* mgr) {
    if (mgr == NULL) return;
    _copy_dma_rx_to_ring(mgr, mgr->dma_rx_buf_size);
    if (mgr->hw_restart_dma_rx_cb) {
        mgr->hw_restart_dma_rx_cb();
    }
}

