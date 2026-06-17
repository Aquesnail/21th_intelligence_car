#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h"
#include "timer.h"
#include "dma_uart.h"
#include <string.h>

#pragma warning disable = 138

volatile uint32_t ulHighFrequencyTimerTicks = 0;
extern uint16_t g_u16OverflowCount ;
TaskHandle_t vTaskMonitor_Handle = NULL;

void ConfigureTimeForRunTimeStats(void)
{
    ulHighFrequencyTimerTicks = 0;
}

void TimDisable(void){
  T9H = 0x00;
  T9L = 0x00;
  ulHighFrequencyTimerTicks = 0;
  TIMER9_Stop();
}

uint32_t GetRunTimeCounterValue(void)
{
    uint16_t u16High;
    uint16_t u16Low;
    unsigned char ucState;

    ucState = EA;
    EA = 0;

    u16High = g_u16OverflowCount;
    u16Low  = ((uint16_t)T9H << 8) | T9L;

    /* 如果定时器溢出标志置位但 ISR 还未执行, 手动补偿 */
    if (TIMER9_CheckFlag() && u16Low < 0x8000u) {
        u16High++;
    }

    EA = ucState;

    return ((uint32_t)u16High << 16) | u16Low;
}

uint8_t xdata taskbit = 1;

uint8_t CPUStatus_HandleCommand(const char* cmd) {
    if (strcmp(cmd, "cpuOn") == 0) {
      taskbit = 1;
      vTaskResume(vTaskMonitor_Handle);
      Debug_Log("COU_ON");
      return 1;
    }else if(strcmp(cmd,"cpuOff") == 0){
      taskbit = 0;
      vTaskSuspend(vTaskMonitor_Handle);
      Debug_Log("CPU_OFF");
      return 1;
    }
    return 0; // 未识别
}

/* 统计信息输出缓冲区，运行时 memset 初始化，避免 Flash 镜像 */
char xdata pcWriteBuffer[512];

void vTaskMonitor(void *pvParameters)
{
    (void)pvParameters;
    vTaskSuspend(NULL); // 刚开始挂起，等待 cpuOn 命令唤醒
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1500));

        if (taskbit == 1)
        {
            vTaskGetRunTimeStats(pcWriteBuffer);

            Debug_Log("\r\n================ CPU Usage ================\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_Log("Task          Abs Time          %% Time\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_Log("-------------------------------------------\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_LogLongString(pcWriteBuffer);
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_Log("===========================================\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));

            vTaskDelay(pdMS_TO_TICKS(1500));
            vTaskList(pcWriteBuffer);
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_Log("\r\n================ Task Status ===============\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_Log("Name          State  Prio  Stack(Min)  Num\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_Log("-------------------------------------------\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_LogLongString(pcWriteBuffer);
            vTaskDelay(pdMS_TO_TICKS(100));
            Debug_Log("===========================================\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void CPU_STATUS_TASK_Init(void){
    /* 运行时初始化 pcWriteBuffer，替代 Flash 镜像 */
    memset(pcWriteBuffer, 0, sizeof(pcWriteBuffer));

    xTaskCreate(
        vTaskMonitor,
        "vTaskMonitor",
        512,
        NULL,
        tskIDLE_PRIORITY + 2,
        &vTaskMonitor_Handle
    );
}

char *taskname = NULL;
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    taskname = pcTaskName;
    (void) xTask;
    while(1){
    };
}

void vApplicationMallocFailedHook( void )
{
    taskDISABLE_INTERRUPTS();
    for( ;; );
}