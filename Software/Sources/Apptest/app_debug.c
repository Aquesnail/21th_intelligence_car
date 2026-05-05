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
debug_port_t        debug_init_structure = {0};
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
    DMA_UART_Write(&uart2_mgr, pData, len);
}

static uint16_t Debug_Read(uint8_t* pData, uint16_t len)
{
    return DMA_UART_Read(&uart2_mgr, pData, len);
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
                256,  // 堆栈大小
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

extern uint8_t CPUStatus_HandleCommand(const char* cmd);
extern uint8_t Storage_HandleCommand(const char* cmd);
char xdata echoBuf[70] = {0};
void Debug_OnTextReceived(char* str, uint8_t len)
{   
    if(Storage_HandleCommand(str)){
        return;
    }
    if(CPUStatus_HandleCommand(str)){
        return;
    }
    
    sprintf(echoBuf, "Echo: %s", str);
    Debug_Log(echoBuf);
}


/* 测试循环任务 */
void vTestLoopTask(void *pvParameters)
{
    char info[64];
    (void)pvParameters;
    // 通知延迟启动任务可以继续了
    xSemaphoreGive(xRegistrationSemaphore);

    Debug_Log("System Booted. Ready for Command.");

    for (;;) {
  
        vTaskDelay(pdMS_TO_TICKS(5));        // 5ms 循环
    }
}