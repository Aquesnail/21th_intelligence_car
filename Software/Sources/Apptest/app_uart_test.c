/*
 * 文件名称: app_uart_test.c
 * 描述: UART4 环形缓冲区回环测试任务
 * 编译标准: C89 (局部变量定义在函数开头)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "dma_uart3.h"
#include <string.h>
#include <stdio.h>

#pragma warning disable = 138
#pragma warning disable = 174
extern dma_uart_manager_t uart3_mgr;
/* 任务句柄与堆栈大小定义 */
#define UART_TEST_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define UART_TEST_STACK_SIZE        ( 512 )

TaskHandle_t xUartTestTaskHandle = NULL;

/* * 环形缓冲区压力测试辅助函数 
 * 描述: 向发送缓冲区大量写入预设字符，测试 DMA 与 RingBuffer 的协同
 */
static void run_ring_buffer_stress_test(void)
{
    /* C89: 变量定义在开头 */
    const char *stress_msg;
    uint16_t msg_len;
    
    stress_msg = "--- Ring Buffer Stress Pattern: [ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789] ---\r\n";
    msg_len = (uint16_t)strlen(stress_msg);

    /* 连续写入两次，触发环形缓冲区跨越边界逻辑 */
    DMA_UART_Write(&uart3_mgr, (const uint8_t *)stress_msg, msg_len);
    DMA_UART_Write(&uart3_mgr, (const uint8_t *)stress_msg, msg_len);
}

/* UART测试任务主函数 */
void vUartTestTask(void *pvParameters)
{
    /* C89: 局部变量定义在函数开头 */
    uint8_t loop_buffer[128];
    uint16_t rx_available;
    uint16_t read_count;
    char tx_buffer[64];
    TickType_t xLastWakeTime;
    TickType_t xCurrentTick;
    const TickType_t xHeartbeatFreq = pdMS_TO_TICKS(1000);
    static uint32_t loop_count = 0;
    
    /* 避免编译器警告 */
    (void)pvParameters;
    
    /* 1. 初始化外设 */
    UART3_Init();
    
    /* 初始化IO (P5.2 RXD4, P5.3 TXD4) */
    // P5M1 &= ~0x0C; 
    // P5M0 |= 0x08;   /* P5.3 (TX4) 推挽输出 */
    // P5PU |= 0x04;   /* P5.2 (RX4) 内部上拉 */
    
    /* 2. 发送测试启动信息 */
    DMA_UART_Write(&uart3_mgr, (const uint8_t *)"\r\n======================================\r\n", 42);
    DMA_UART_Write(&uart3_mgr, (const uint8_t *)"  UART4 Ring Buffer Loopback Test   \r\n", 40);
    DMA_UART_Write(&uart3_mgr, (const uint8_t *)"======================================\r\n", 42);
    
    /* 执行一次简单的压力输出测试 */
    run_ring_buffer_stress_test();
    
    DMA_UART_Write(&uart3_mgr, (const uint8_t *)"\r\nSystem ready. Waiting for data echo...\r\n", 42);
    
    /* 获取当前Tick作为基准 */
    xLastWakeTime = xTaskGetTickCount();
    
    /* 3. 任务主循环 */
    for (;;)
    {
        /* --- 核心回环测试逻辑 --- */
        /* 检查环形缓冲区中是否有待处理数据 */
        rx_available = DMA_UART_Available(&uart3_mgr);
        
        if (rx_available > 0)
        {
            /* 每次最多读取 loop_buffer 的大小 */
            if (rx_available > sizeof(loop_buffer))
            {
                rx_available = sizeof(loop_buffer);
            }
            
            /* 从 RingBuffer 读取数据 (DMA接收的内容已由中断存入此处) */
            read_count = DMA_UART_Read(&uart3_mgr, loop_buffer, rx_available);
            
            if (read_count > 0)
            {
                /* 将读取到的数据原样写回发送缓冲区 (将触发 DMA 发送) */
                DMA_UART_Write(&uart3_mgr, loop_buffer, read_count);
            }
        }
        
        /* --- 定时心跳监控 --- */
        xCurrentTick = xTaskGetTickCount();
        
        if ((xCurrentTick - xLastWakeTime) >= xHeartbeatFreq)
        {
            xLastWakeTime = xCurrentTick;
            loop_count++;
            
            /* 格式化并发送心跳包，监控任务存活状态 */
            sprintf(tx_buffer, "[Alive:%lu] Ring-Free-TX:%u\r\n", 
                    (unsigned long)loop_count,
                    (unsigned int)(uart3_mgr.ring_tx_buf_size - uart3_mgr.tx_count));
            
            DMA_UART_Write(&uart3_mgr, (const uint8_t *)tx_buffer, (uint16_t)strlen(tx_buffer));
        }
        
        /* 让出 CPU，给 DMA 中断留出处理时间 */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* 初始化测试任务 */
void App_UartTest_Init(void)
{
    xTaskCreate(vUartTestTask,          /* 任务函数 */
                "UartTest",             /* 任务名称 */
                UART_TEST_STACK_SIZE,   /* 堆栈大小 */
                NULL,                   /* 参数 */
                UART_TEST_TASK_PRIORITY,/* 优先级 */
                &xUartTestTaskHandle);  /* 句柄 */
}