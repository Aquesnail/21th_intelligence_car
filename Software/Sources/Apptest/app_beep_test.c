#include "FreeRTOS.h"
#include "task.h"
#include "beep_port.h"
#include "music.h"  /* 包含生成的音乐数据 */
#include "debug_arch.h"

/* 如果你配置了串口重定向，可以取消注释 stdio.h 来使用 printf */
/* #include <stdio.h> */

/* 定义一个全局的 volatile 变量，方便在没有串口时通过 IDE 调试器实时查看耗时 */
volatile uint32_t g_beep_push_cost_ticks = 0;
TaskHandle_t App_BeepStressTestTask_Handle;
void App_BeepStressTestTask(void *pvParameters) {
    TickType_t start_tick;
    TickType_t end_tick;
    int i;  /* 循环变量，符合 C89 要求在函数开头声明 */

    (void)pvParameters;
    vTaskSuspend(App_BeepStressTestTask_Handle);
    while (1) {
        /* 1. 记录开始推送音符前的时间戳 */
        start_tick = xTaskGetTickCount();

        /* 2. 连续压入所有音乐音符 */
        for (i = 0; i < melody_length; i++) {
            Beep_Out(0, melody[i], durations[i]);
        }

        /* 3. 记录推送完成后的时间戳，并计算耗时 */
        end_tick = xTaskGetTickCount();
        g_beep_push_cost_ticks = (uint32_t)(end_tick - start_tick);

        /* 4. 让当前测试任务休眠 3 秒（可根据音乐实际时长调整） */
        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}

uint8_t Beep_HandleCommand(const char* cmd) {
    if (strcmp(cmd, "musicOn") == 0) {
        vTaskResume(App_BeepStressTestTask_Handle);
        Debug_Log("Music: On\r\n");
        return 1;
    } else if(strcmp(cmd,"musicOff") == 0){
        Debug_Log("Music: Off\r\n");
        vTaskSuspend(App_BeepStressTestTask_Handle);
        return 1;
    }
    return 0; // 未识别
}

void BeepTestTask_Init(void) {
    xTaskCreate(App_BeepStressTestTask, "BeepStress", 256+128, NULL, tskIDLE_PRIORITY + 1, &App_BeepStressTestTask_Handle);
}