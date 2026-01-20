/*
 * 文件名称: app_uart_test.c
 * 描述: UART5 功能测试任务，包含简单的CRC硬件验证
 * 编译标准: C99
 */

#include "FreeRTOS.h"
#include "task.h"
#include "dma_uart.h"
#include "crc.h"
#include <string.h>
#include <stdio.h>

#pragma warning disable = 138

/* 任务句柄与堆栈大小定义 */
#define UART_TEST_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define UART_TEST_STACK_SIZE        ( 512 )

TaskHandle_t xUartTestTaskHandle = NULL;

/* CRC测试数据结构 */
typedef struct {
    const char *name;
    const uint8_t *dat;
    uint16_t length;
    uint16_t expected_crc;
} crc_test_case_t;

/* MODBUS CRC标准测试数据 */
static const uint8_t test_dat1[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
static const uint8_t test_dat2[] = {0x01, 0x10, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x0A, 0x01, 0x02};
static const uint8_t test_dat3[] = "123456789";

/* CRC测试用例表 */
static const crc_test_case_t crc_test_cases[] = {
    {"MODBUS读取10个寄存器", test_dat1, sizeof(test_dat1), 0xCDC5},
    {"MODBUS写保持寄存器",   test_dat2, sizeof(test_dat2), 0x3092},
    {"ASCII字符串123456789", test_dat3, sizeof(test_dat3)-1, 0x4B37},
};

#define CRC_TEST_CASE_COUNT  (sizeof(crc_test_cases) / sizeof(crc_test_cases[0]))

/* CRC验证测试函数 */
static void run_crc_verification_test(void)
{
    uint8_t all_pass = 1;
    char result_msg[128];
    uint8_t i = 0;
    uint32_t hw_crc = 0;
    uint16_t hw_crc_16 = 0;
    const crc_test_case_t *test = NULL;
    uint8_t pass = 0;
    
    /* 发送测试开始信息 */
    UART5_Write((uint8_t *)"\r\n=== CRC Hardware Verification Test ===\r\n", 44);
    
    /* 初始化CRC模块 */
    CRC_Init();
    
    /* 运行所有测试用例 */
    for (i = 0; i < CRC_TEST_CASE_COUNT; i++)
    {
        test = &crc_test_cases[i];
        
        /* 计算硬件CRC */
        hw_crc = CRC_Calculate((uint8_t *)test->dat, test->length);
        hw_crc_16 = hw_crc & 0xFFFF;
        
        /* 判断结果 */
        pass = (hw_crc_16 == test->expected_crc);
        
        if (!pass)
        {
            all_pass = 0;
        }
        
        /* 格式化并发送测试结果 */
        sprintf(result_msg, "Test %d [%s]:\r\n"
                           "  Expected: 0x%04X\r\n"
                           "  Actual:   0x%04X\r\n"
                           "  Result:   %s\r\n\r\n",
                           i+1, test->name,
                           test->expected_crc,
                           hw_crc_16,
                           pass ? "PASS" : "FAIL");
        
        UART5_Write((uint8_t *)result_msg, strlen(result_msg));
        
        /* 短暂延时 */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    /* 显示最终结果 */
    if (all_pass)
    {
        UART5_Write((uint8_t *)"=== All tests PASSED! ===\r\n\r\n", 32);
    }
    else
    {
        UART5_Write((uint8_t *)"!!! Some tests FAILED !!!\r\n\r\n", 32);
    }
}
static void verify_crc_hardware(void)
{
	  uint8_t test1[] = {0x12, 0x34};
    uint8_t test2[] = {0xFF, 0xFF};
    uint8_t test3[] = {0x00, 0x00};
    uint32_t crc1, crc2, crc3;
    char msg[64];
    
    UART5_Write((uint8_t *)"\r\n=== Simple CRC Test ===\r\n", 28);
    
    // 测试1: 0x12 0x34
    CRC_Init();  // 使用您在config.h中的初始化
    crc1 = CRC_Calculate(test1, 2);
    sprintf(msg, "Data 0x12 0x34 -> CRC: 0x%04X\r\n", crc1 & 0xFFFF);
    UART5_Write((uint8_t *)msg, strlen(msg));
    
    // 测试2: 0xFF 0xFF
    CRC_Init();
    crc2 = CRC_Calculate(test2, 2);
    sprintf(msg, "Data 0xFF 0xFF -> CRC: 0x%04X\r\n", crc2 & 0xFFFF);
    UART5_Write((uint8_t *)msg, strlen(msg));
    
    // 测试3: 0x00 0x00
    CRC_Init();
    crc3 = CRC_Calculate(test3, 2);
    sprintf(msg, "Data 0x00 0x00 -> CRC: 0x%04X\r\n", crc3 & 0xFFFF);
    UART5_Write((uint8_t *)msg, strlen(msg));
}
/* UART测试任务主函数 */
void vUartTestTask(void *pvParameters)
{
    /* 所有变量定义在函数开头（C99标准） */
    uint8_t echo_buffer[64];
    uint16_t rx_len = 0;
    uint16_t i = 0;
    uint8_t ch = 0;
    char tx_buffer[64];
    static uint32_t loop_count = 0;
    TickType_t xLastWakeTime = 0;
    TickType_t xCurrentTick = 0;
    const TickType_t xHeartbeatFreq = pdMS_TO_TICKS(1000);
    
    /* 避免编译器警告 */
    (void)pvParameters;
    
    /* 1. 初始化外设 */
    UART5_Init();
    CRC_Init();
    
    /* 初始化IO */
    P0M1 = 0x00; 
    P0M0 = 0x20;   /* P0.5 推挽输出 */
    P0PU |= 0x10;  /* P0.4 内部上拉 */
    
    /* 发送启动信息 */
    UART5_Write((uint8_t *)"\r\n=== UART5 Test Start ===\r\n", 28);
    
    /* 2. 运行CRC验证测试（只运行一次） */
    run_crc_verification_test();
    //verify_crc_hardware();
    /* 3. 发送操作提示 */
    UART5_Write((uint8_t *)"System started. Type anything for echo test.\r\n", 45);
    UART5_Write((uint8_t *)"Heartbeat will be sent every 1 second.\r\n\r\n", 42);
    
    /* 获取当前Tick作为基准 */
    xLastWakeTime = xTaskGetTickCount();
    
    /* 4. 任务主循环 */
    for (;;)
    {
        /* 功能A: 回显服务 */
        rx_len = UART5_Available();
        if (rx_len > 0)
        {
            /* 限制读取长度 */
            if (rx_len > sizeof(echo_buffer))
            {
                rx_len = sizeof(echo_buffer);
            }
            
            /* 读取数据 */
            UART5_Read(echo_buffer, rx_len);
            
            /* 回显接收到的数据 */
            UART5_Write(echo_buffer, rx_len);
        }
        
        /* 功能B: 定时心跳 */
        xCurrentTick = xTaskGetTickCount();
        
        /* 检查是否到达1秒时间点 */
        if ((xCurrentTick - xLastWakeTime) >= xHeartbeatFreq)
        {
            /* 更新上次唤醒时间 */
            xLastWakeTime = xCurrentTick;
            loop_count++;
            
            /* 格式化心跳包 */
            sprintf(tx_buffer, "[%lu] System running...\r\n", (unsigned long)loop_count);
            
            /* 发送心跳 */
            UART5_Write((uint8_t *)tx_buffer, strlen(tx_buffer));
        }
        
        /* 任务调度让步 */
        vTaskDelay(pdMS_TO_TICKS(2));
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