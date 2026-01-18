/*
 * 文件名称: app_uart_test.c
 * 描述: UART5 功能测试任务
 */

#include "FreeRTOS.h"
#include "task.h"
#include "dma_uart.h"
#include <string.h>
#include <stdio.h>

#pragma warning disable = 138

// ==========================================
// 任务句柄与堆栈大小定义
// ==========================================
#define UART_TEST_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define UART_TEST_STACK_SIZE        ( 512 ) // 根据实际FreeRTOS配置调整

TaskHandle_t xUartTestTaskHandle = NULL;
/**
 * @brief  UART 测试任务
 * @param  pvParameters: 任务参数
 * @retval None
 */
void vUartTestTask(void *pvParameters)
{
    // ============================================================
    // C99: 所有变量必须在函数最开头定义
    // ============================================================
    
    // Echo (回显) 功能专用变量
    uint8_t echo_buffer[64];    // 临时接收缓冲区
    uint16_t rx_len;            // 本次接收到的长度

    // Heartbeat (心跳) 功能专用变量
    char tx_buffer[64];
    static uint32_t loop_count = 0;
    
    // FreeRTOS 时间管理变量
    TickType_t xLastWakeTime;
    TickType_t xCurrentTick;
    const TickType_t xHeartbeatFreq = pdMS_TO_TICKS(1000); // 1秒心跳间隔

    // 避免编译器警告
    (void)pvParameters;

    // ============================================================
    // 1. 初始化
    // ============================================================
    UART5_Init();
    
    // 初始化 IO (根据你的代码保留)
    P0M1 = 0x00; P0M0 = 0x20;   // P0.5 推挽输出
    P0PU |= 0x10;               // P0.4 内部上拉

    // 发送启动信息
    UART5_Write((uint8_t *)"\r\n--- UART5 Loopback Test Start ---\r\n", 35);

    // 获取当前 Tick 作为基准
    xLastWakeTime = xTaskGetTickCount();

    // ============================================================
    // 2. 任务主循环
    // ============================================================
    for (;;)
    {
        // ---------------------------------------------------------
        // 功能 A: 回显服务 (Echo Service)
        // 逻辑: 只要 RX RingBuffer 里有数据，就读出来原样发回去
        // ---------------------------------------------------------
        
        // 1. 查询有多少数据可读
        rx_len = UART5_Available();

        if (rx_len > 0)
        {
            // 2. 限制单次读取长度，防止超过临时缓冲区大小
            if (rx_len > sizeof(echo_buffer)) 
            {
                rx_len = sizeof(echo_buffer);
            }

            // 3. 从 RingBuffer 读取数据
            UART5_Read(echo_buffer, rx_len);

            // 4. 立即原样发送回去 (关键测试步骤)
            UART5_Write(echo_buffer, rx_len);
        }

        // ---------------------------------------------------------
        // 功能 B: 定时心跳 (Heartbeat) - 非阻塞检查
        // ---------------------------------------------------------
        xCurrentTick = xTaskGetTickCount();

        // 检查是否到达 1秒 时间点
        // 注意：使用无符号减法 (Current - Last) 处理 Tick 溢出是安全的
        if ((xCurrentTick - xLastWakeTime) >= xHeartbeatFreq)
        {
            // 更新上次唤醒时间
            xLastWakeTime = xCurrentTick;
            loop_count++;

            // 格式化心跳包 (建议使用 sprintf，比手写安全且标准)
            sprintf(tx_buffer, "Heartbeat: %lu\r\n", (unsigned long)loop_count);
            
            // 发送心跳
            UART5_Write((uint8_t *)tx_buffer, strlen(tx_buffer));
        }

        // ---------------------------------------------------------
        // 任务调度让步
        // ---------------------------------------------------------
        // 这是一个死循环，如果没有任何延时，低优先级任务会被饿死。
        // 加一个极短的延时 (2ms)，既不影响串口吞吐体验，又能释放 CPU。
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}
/**
 * @brief  初始化测试任务
 */
void App_UartTest_Init(void)
{
    // 确保 UART 已经初始化
    UART5_Init(); // 如果 main 中已调用，这里不需要

    xTaskCreate(vUartTestTask,          // 任务函数
                "UartTest",             // 任务名称
                UART_TEST_STACK_SIZE,   // 堆栈大小
                NULL,                   // 参数
                UART_TEST_TASK_PRIORITY,// 优先级
                &xUartTestTaskHandle);  // 句柄
}