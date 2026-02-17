#include "debug_arch.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
//#include "debug.h" // 包含刚才写的 debug 模块
#include <stdio.h>
#include "delay.h"
#define DEBUG_TASK_STACK_SIZE 512
#define DEBUG_TASK_PRIORITY (tskIDLE_PRIORITY + 2 )

#pragma warning disable = 138
#pragma warning disable = 47

TaskHandle_t xDebugTaskHandle = NULL;
TaskHandle_t xDebugTaskHandle2 = NULL;
SemaphoreHandle_t xRegistrationSemaphore = NULL;
void vDebugTaskDelayedStart(void *pvParameters);
void vTestLoopTask(void *pvParameters);
void App_DebugTask_Init(void){
    // 创建注册信号量
    xRegistrationSemaphore = xSemaphoreCreateBinary();
    if(xRegistrationSemaphore == NULL) {
        // 处理信号量创建失败
        while(1);
    }
      xTaskCreate(vTestLoopTask,          // 任务函数
                "Debug_Test",             // 任务名称
                DEBUG_TASK_STACK_SIZE,   // 堆栈大小
                NULL,                   // 参数
                DEBUG_TASK_PRIORITY,// 优先级
                &xDebugTaskHandle2);  // 句柄
     xTaskCreate(vDebugTaskDelayedStart,
                "DebugStarter",
                configMINIMAL_STACK_SIZE,  // 只需要很小的栈
                NULL,
                DEBUG_TASK_PRIORITY - 1,   // 低优先级
                NULL);
    // xTaskCreate(vDebugTask,          // 任务函数
    //             "UartTest",             // 任务名称
    //             DEBUG_TASK_STACK_SIZE,   // 堆栈大小
    //             NULL,                   // 参数
    //             DEBUG_TASK_PRIOTITY,// 优先级
    //             &xDebugTaskHandle);  // 句柄
  
}
void vDebugTaskDelayedStart(void *pvParameters) {
    (void)pvParameters;
    // 延时等待系统稳定
   //vTaskDelay(pdMS_TO_TICKS(2000)); // 延时2秒
    Debug_Init();
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
void Debug_OnTextReceived(char* str, uint8_t len) {
    char echoBuf[70];
    
    // 可以在这里处理命令，比如:
    // if(strcmp(str, "reboot") == 0) IAP_CONTR = 0x20;
    
    // --- 回环测试 ---
    // 加上前缀 "Echo: " 发回去
    sprintf(echoBuf, "Echo: %s", str);
    
    // 调用新的日志发送函数
    Debug_Log(echoBuf);
}
// --- 定义测试变量 (全部放在 xdata) ---
// 1. 普通计数器 (测试 u16)
static uint16_t test_counter = 0;
// 2. 模拟正弦波 (测试 float)
static float test_sine = 0.0;
// 3. 控制目标值 (测试上位机下行修改功能)
static uint8_t target_val = 10;
// 4. [隐藏变量] 不在启动时注册，等待上位机动态添加 (测试 0x56 指令)
static uint32_t hidden_secret = 0x12345678;

static float test_sine2 = 0.0;
// 辅助函数：简单的正弦模拟
float fake_sin(uint16_t step) {
    // 简化的周期变化，非标准sin
    int16_t v = (step % 360);
    if(v > 180) v = 360 - v;
    return (float)v / 180.0f; 
}

void vTestLoopTask(void *pvParameters) {
    char info[64];
    (void)pvParameters;

    // 1. [启动注册] 注册部分变量，故意漏掉 hidden_secret
    Debug_Register("Counter", DBG_UINT16|LOW_FREQ_TYPE, &test_counter);
    Debug_Register("SineWave", DBG_FLOAT|HIGH_FREQ_TYPE, &test_sine);
		Debug_Register("SineWave2", DBG_FLOAT|HIGH_FREQ_TYPE, &test_sine2);
    Debug_Register("Control", DBG_UINT8|LOW_FREQ_TYPE, &target_val);
		Debug_Register("Counter", DBG_UINT16|LOW_FREQ_TYPE, &test_counter);
		Debug_Register("Counter", DBG_UINT16|LOW_FREQ_TYPE, &test_counter);
		Debug_Register("Counter", DBG_UINT16|HIGH_FREQ_TYPE, &test_counter);
		Debug_Register("Counter", DBG_UINT16|LOW_FREQ_TYPE, &test_counter);

    xSemaphoreGive(xRegistrationSemaphore);
    // 2. [重要] 打印隐藏变量的物理地址
    // 因为是动态分配，我们需要知道它的地址才能在 Python 里填对
    // 注意：STC32 C251 指针打印出来通常包含类型字节
    // %p 在 C251 下行为不一，这里手动打印地址方便你调试
    {
        //uint8_t *p = (uint8_t *)&hidden_secret;
        // p[0]=类型(01:xdata), p[1]=高8位, p[2]=低8位
        //sprintf(info, "\r\n[TEST] Hidden Var Addr: 0x%02X%02X%02X\r\n", 
         //       (unsigned int)p[0], (unsigned int)p[1], (unsigned int)p[2]);
        //UART5_Write((uint8_t*)info, strlen(info));
    }
    Debug_Log("System Booted. Ready for Text Test.");
    for (;;) {
        // --- 模拟业务逻辑 ---
        
        // 1. 计数器累加
        test_counter++;

        // 2. 正弦波更新
        test_sine = fake_sin(test_counter);
				test_sine2 = test_sine+1.0;
        // 3. 隐藏变量更新 (验证动态注册后能否看到它在变)
        hidden_secret += 1;

        // 4. 使用 target_val (模拟被控对象)
        // 如果上位机改了这个值，我们在串口打印出来证明生效了
       // if (test_counter % 20 == 0) { // 每2秒打印一次
      //      sprintf(info, "[LOG] Current Target: %bu\r\n", target_val);
      //      UART5_Write((uint8_t*)info, strlen(info));
       // }

        vTaskDelay(pdMS_TO_TICKS(5)); // 100ms 更新一次
    }
}