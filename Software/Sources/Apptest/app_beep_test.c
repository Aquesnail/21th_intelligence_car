#include "FreeRTOS.h"
#include "task.h"
#include "beep_port.h"

/* 如果你配置了串口重定向，可以取消注释 stdio.h 来使用 printf */
/* #include <stdio.h> */

/* 定义一个全局的 volatile 变量，方便在没有串口时通过 IDE 调试器实时查看耗时 */
volatile uint32_t g_beep_push_cost_ticks = 0;

void App_BeepStressTestTask(void *pvParameters) {
    TickType_t start_tick;
    TickType_t end_tick;
    uint8_t i;
    uint16_t freq;

    (void)pvParameters;

    while (1) {
        /* 1. 记录开始推送音符前的时间戳 */
        start_tick = xTaskGetTickCount();

        /* 2. 连续压入 100 个音符 (模拟一个频率从 500Hz 到 2480Hz 的滑音/扫频音效) */
        for (i = 0; i < 100; i++) {
            freq = 500 + (i * 20); 
            
            /* 向通道 0 压入音符：每个音符响 20ms。
             * 理论上 100 个音符总共需要 100 * 20ms = 2000ms (2秒) 才能在物理层播完。
             */
            Beep_Out(0, freq, 20); 
        }

        /* 3. 记录推送完成后的时间戳，并计算耗时 */
        end_tick = xTaskGetTickCount();
        g_beep_push_cost_ticks = (uint32_t)(end_tick - start_tick);

        /* * 【预期现象】：
         * g_beep_push_cost_ticks 的值应该为 0 或 1（取决于调度器精度和运行时的时钟节拍）。
         * 这意味着 100 个音符瞬间塞入了 xdata 二级缓冲区，当前任务几乎没花时间，毫无卡顿！
         * 同时，你的蜂鸣器会在接下来的 2 秒内，由后台任务平滑地播放出扫频音效。
         */

        /* 如果有串口，可以打印出来看 */
        /* printf("Pushed 100 notes in %lu ticks!\r\n", g_beep_push_cost_ticks); */

        /* 4. 让当前测试任务休眠 3 秒。
         * 这期间 CPU 归还给 FreeRTOS，后台的 BeepHandler 任务和硬件定时器会接管工作。
         */
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    
}

void BeepTestTask_Init(){
    xTaskCreate(App_BeepStressTestTask, "BeepStress", 256+128, NULL, tskIDLE_PRIORITY + 1, NULL);
}