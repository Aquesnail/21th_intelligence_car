/*
 * 文件名称: app_uart_test.c
 * 描述: UART5 功能测试任务
 */

#include "FreeRTOS.h"
#include "task.h"
#include "dma_uart.h"
#include <string.h>
#include <stdio.h>

// ==========================================
// 任务句柄与堆栈大小定义
// ==========================================
#define UART_TEST_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define UART_TEST_STACK_SIZE        ( 256 ) // 根据实际FreeRTOS配置调整

TaskHandle_t xUartTestTaskHandle = NULL;
/**
 * @brief  UART 测试任务
 * @param  pvParameters: 任务参数
 * @retval None
 */
static void int_to_str(uint32_t num, char *str) {
    char temp[12];
    int i = 0;
    int j = 0;
    
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while (num > 0) {
        temp[i++] = (num % 10) + '0';
        num /= 10;
    }

    // 反转字符串
    for (j = 0; j < i; j++) {
        str[j] = temp[i - 1 - j];
    }
    str[i] = '\0';
}
void vUartTestTask(void *pvParameters)
{
    // C99: 变量必须在函数开头定义
    uint8_t rx_byte;
    char tx_buffer[64];
    char num_str[12];
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1秒心跳
    TickType_t xCurrentTick;
    static uint32_t loop_count = 0;

    // 避免编译器警告未使用参数
    (void)pvParameters;

    // 1. 初始化 UART5
    UART5_Init();

    // 2. 发送启动欢迎信息
    UART5_Write((uint8_t *)"\r\n============================\r\n", 32);
    UART5_Write((uint8_t *)"   STC32 DMA UART RingBuffer\r\n", 31);
    UART5_Write((uint8_t *)"   FreeRTOS Test Task Start\r\n", 30);
    UART5_Write((uint8_t *)"============================\r\n", 32);

    // 获取当前 Tick
    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        // ---------------------------------------------------------
        // 功能 B: 定时心跳包 (Heartbeat) - 每1秒发送一次
        // ---------------------------------------------------------
        xCurrentTick = xTaskGetTickCount();
        
        // 检查是否距离上次发送心跳超过了1秒
        // 注意：这里没有使用 vTaskDelayUntil 阻塞整个循环，
        // 因为我们还需要上面的 Echo 逻辑保持响应。
        if ((xCurrentTick - xLastWakeTime) >= xFrequency)
        {
            xLastWakeTime = xCurrentTick;
            loop_count++;

            // 构造字符串: "Heartbeat: 123 ticks\r\n"
            // 如果没有 sprintf，我们手动拼装
            strcpy(tx_buffer, "Time: ");
            int_to_str(loop_count, num_str);
            strcat(tx_buffer, num_str);
            strcat(tx_buffer, "\r\n");

            // 发送
            UART5_Write((uint8_t *)tx_buffer, strlen(tx_buffer));
            
        }
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