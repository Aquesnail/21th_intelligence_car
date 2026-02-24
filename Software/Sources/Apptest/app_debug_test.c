#include "debug_arch.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
// #include "debug.h"          // 包含调试模块
#include <stdio.h>
#include "delay.h"
#include "dma_uart_ring.h"
#include "dma_uart.h"

#define DEBUG_TASK_STACK_SIZE   512
#define DEBUG_TASK_PRIORITY     (tskIDLE_PRIORITY + 2)

#pragma warning disable = 138
#pragma warning disable = 47

/* 全局变量 ---------------------------------------------------------------- */
debug_port_t        debug_init_structure;
TaskHandle_t        xDebugTaskHandle    = NULL;
TaskHandle_t        xDebugTaskHandle2   = NULL;
SemaphoreHandle_t   xRegistrationSemaphore = NULL;

/* 私有函数原型 ------------------------------------------------------------ */
static void Debug_Write(uint8_t* pData, uint16_t len);
static uint16_t Debug_Read(uint8_t* pData, uint16_t len);
void vDebugTaskDelayedStart(void *pvParameters);
void vTestLoopTask(void *pvParameters);

/* 私有函数实现 ------------------------------------------------------------ */
static void Debug_Write(uint8_t* pData, uint16_t len)
{
    DMA_UART_Write(&uart5_mgr, pData, len);
}

static uint16_t Debug_Read(uint8_t* pData, uint16_t len)
{
    return DMA_UART_Read(&uart5_mgr, pData, len);
}

/* 公共函数 ---------------------------------------------------------------- */
void App_DebugTask_Init(void)
{
    // 创建注册信号量
    xRegistrationSemaphore = xSemaphoreCreateBinary();
    if (xRegistrationSemaphore == NULL) {
        // 信号量创建失败，死循环
        while (1);
    }

    // 创建测试循环任务
    xTaskCreate(vTestLoopTask,          // 任务函数
                "Debug_Test",           // 任务名称
                DEBUG_TASK_STACK_SIZE,  // 堆栈大小
                NULL,                   // 参数
                DEBUG_TASK_PRIORITY,    // 优先级
                &xDebugTaskHandle2);    // 任务句柄

    // 创建延迟启动任务（低优先级）
    xTaskCreate(vDebugTaskDelayedStart,
                "DebugStarter",
                configMINIMAL_STACK_SIZE,
                NULL,
                DEBUG_TASK_PRIORITY - 1,
                NULL);
}

void vDebugTaskDelayedStart(void *pvParameters)
{
    (void)pvParameters;

    // 延时等待系统稳定（可选项，当前被注释）
    // vTaskDelay(pdMS_TO_TICKS(2000));

    debug_init_structure.debug_read  = Debug_Read;
    debug_init_structure.debug_write = Debug_Write;
    Debug_Init(&debug_init_structure);

    if (xSemaphoreTake(xRegistrationSemaphore, portMAX_DELAY) == pdTRUE) {
        // 注册完成后创建真正的调试任务
        xTaskCreate(vDebugTask,
                    "DebugTask",
                    DEBUG_TASK_STACK_SIZE,
                    NULL,
                    DEBUG_TASK_PRIORITY,
                    &xDebugTaskHandle);
    }
    xSemaphoreGive(xRegistrationSemaphore);

    // 删除自身任务
    vTaskDelete(NULL);
}

void Debug_OnTextReceived(char* str, uint8_t len)
{
    char echoBuf[70];

    // 可在此处理命令，例如：
    // if (strcmp(str, "reboot") == 0) IAP_CONTR = 0x20;

    // 回环测试：加上前缀发回
    sprintf(echoBuf, "Echo: %s", str);
    Debug_Log(echoBuf);
}

/* 测试变量（全部放在 xdata）----------------------------------------------- */
static uint16_t test_counter = 0;       // 普通计数器
static float   test_sine     = 0.0;      // 模拟正弦波
static uint8_t target_val    = 10;       // 控制目标值（上位机可改）
static uint32_t hidden_secret = 0x12345678; // 隐藏变量，用于动态注册测试
static float   test_sine2    = 0.0;

/* 辅助函数：简单的正弦模拟 */
float fake_sin(uint16_t step)
{
    int16_t v = (step % 360);
    if (v > 180) v = 360 - v;
    return (float)v / 180.0f;
}

/* 测试循环任务 */
void vTestLoopTask(void *pvParameters)
{
    char info[64];
    (void)pvParameters;

    // 启动时注册变量（此处仅演示注释，实际可根据需要打开）
    // Debug_Register("Counter",  DBG_UINT16 | LOW_FREQ_TYPE,  &test_counter);
    // Debug_Register("SineWave", DBG_FLOAT  | HIGH_FREQ_TYPE, &test_sine);
    // Debug_Register("SineWave2",DBG_FLOAT  | HIGH_FREQ_TYPE, &test_sine2);
    // Debug_Register("Control",  DBG_UINT8  | LOW_FREQ_TYPE,  &target_val);
    // Debug_Register("Counter",  DBG_UINT16 | LOW_FREQ_TYPE,  &test_counter);
    // Debug_Register("Counter",  DBG_UINT16 | LOW_FREQ_TYPE,  &test_counter);
    // Debug_Register("Counter",  DBG_UINT16 | HIGH_FREQ_TYPE, &test_counter);
    // Debug_Register("Counter",  DBG_UINT16 | LOW_FREQ_TYPE,  &test_counter);

    // 通知延迟启动任务可以继续了
    xSemaphoreGive(xRegistrationSemaphore);

    // 隐藏变量地址打印（注释掉，需要时可打开）
    // {
    //     uint8_t *p = (uint8_t *)&hidden_secret;
    //     sprintf(info, "\r\n[TEST] Hidden Var Addr: 0x%02X%02X%02X\r\n",
    //             (unsigned int)p[0], (unsigned int)p[1], (unsigned int)p[2]);
    //     UART5_Write((uint8_t*)info, strlen(info));
    // }

    Debug_Log("System Booted. Ready for Command.");

    for (;;) {
        // 模拟业务逻辑
        test_counter++;                     // 计数器累加

        test_sine = fake_sin(test_counter); // 正弦波更新
        test_sine2 = test_sine + 1.0;

        hidden_secret += 1;                  // 隐藏变量变化

        // 可选的 target_val 变化打印（注释掉）
        // if (test_counter % 20 == 0) {
        //     sprintf(info, "[LOG] Current Target: %bu\r\n", target_val);
        //     UART5_Write((uint8_t*)info, strlen(info));
        // }

        vTaskDelay(pdMS_TO_TICKS(5));        // 5ms 循环
    }
}