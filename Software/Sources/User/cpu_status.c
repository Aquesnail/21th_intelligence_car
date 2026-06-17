#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h"

#pragma warning disable = 138

uint8_t xdata taskbit = 1;

uint8_t CPUStatus_HandleCommand(const char* cmd) {
    if (strcmp(cmd, "cpuOn") == 0) {
      taskbit = 1;
      Debug_Log("COU_ON");
      return 1;
    }else if(strcmp(cmd,"cpuOff") == 0){
      taskbit = 0;
      Debug_Log("CPU_OFF");
      return 1;
    }
    return 0;
}

/* 运行时统计已关闭，监控任务已移除 */
void CPU_STATUS_TASK_Init(void)
{
    (void)0;
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