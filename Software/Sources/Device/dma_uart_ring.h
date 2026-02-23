#ifndef _DMA_UART_RING_H
#define _DMA_UART_RING_H

#include <stdint.h>
#include <stddef.h>
#include "config.h"
/* ==========================================
 * 结构体定义
 * ========================================== */

typedef struct {
    /* --- 1. 物理 DMA 缓冲区 --- */
    uint8_t* dma_tx_buf;
    uint16_t dma_tx_buf_size;
    
    uint8_t* dma_rx_buf;
    uint16_t dma_rx_buf_size;

    /* --- 2. 软件逻辑 环形缓冲区 --- */
    uint8_t* ring_tx_buf;
    uint16_t ring_tx_buf_size;

    uint8_t* ring_rx_buf;
    uint16_t ring_rx_buf_size;

    /* --- 3. 缓冲区指针与状态 (内部维护，外部勿动) --- */
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;
    volatile uint16_t tx_count;

    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    volatile uint16_t rx_count;

    volatile uint8_t is_dma_tx_busy;

    /* --- 4. 硬件层回调函数指针 --- */
    /* 当软件将数据搬运到 DMA TX Buffer 后，调用此函数启动硬件 DMA 发送 */
    void (*hw_start_dma_tx_cb)(uint16_t bytes_to_send);
    
    /* 当软件读走 DMA RX Buffer 的数据后，调用此函数重新启动硬件 DMA 接收 */
    void (*hw_restart_dma_rx_cb)(void);

    /* --- 5. 操作系统层回调函数指针 (解耦 FreeRTOS) --- */
    void (*os_enter_critical_cb)(void);
    void (*os_exit_critical_cb)(void);
    void (*os_delay_cb)(uint32_t ms);

} dma_uart_manager_t;


/* ==========================================
 * 外部接口函数声明
 * ========================================== */

/* 初始化管理器状态 */
void DMA_UART_Manager_Init(dma_uart_manager_t* mgr);

/* 读写数据接口 (供应用层调用) */
uint16_t DMA_UART_Write(dma_uart_manager_t* mgr, const uint8_t* dat, uint16_t len);
uint16_t DMA_UART_Read(dma_uart_manager_t* mgr, uint8_t* dat, uint16_t len);
uint16_t DMA_UART_Available(dma_uart_manager_t* mgr);

/* 中断事件处理接口 (供硬件 ISR 调用) */
void DMA_UART_OnTxDone_ISR(dma_uart_manager_t* mgr);
void DMA_UART_OnRxTimeout_ISR(dma_uart_manager_t* mgr, uint16_t received_len);
void DMA_UART_OnRxFull_ISR(dma_uart_manager_t* mgr);

#endif /* _DMA_UART_RING_H */