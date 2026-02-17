#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h"
#include "timer.h"
/* 在你的 C 文件中实现这两个函数 */

volatile uint32_t ulHighFrequencyTimerTicks = 0;
extern uint16_t g_u16OverflowCount ;
// 初始化定时器 (这只是伪代码，请使用 HAL 库或寄存器配置实际硬件)
void ConfigureTimeForRunTimeStats(void)
{
    // 1. 开启 TIM2 时钟
    // 2. 配置 Prescaler 和 Period
    // 3. 启动定时器: HAL_TIM_Base_Start(&htim2);
    // 注意：这里仅仅启动计数即可，不需要开启更新中断，除非定时器位数不够需要溢出处理
    TIMER11_Init();
    ulHighFrequencyTimerTicks = 0;
}

// 获取当前时间值
uint32_t GetRunTimeCounterValue(void)
{
    uint32_t ulReturn;
    uint16_t u16High;
    uint16_t u16Low;
    
    // 【关键】进入临界区，防止读取过程中发生中断导致高低位不匹配
    // 比如：读了 Low 是 0xFFFF，刚准备读 High，中断来了 High+1，
    // 回来后读到新的 High，拼起来就会导致时间“突变”大了 65536。
    // FreeRTOS 提供了临界区保护函数：
    unsigned char ucState = 0;
    
    // 简单的关中断保护 (STC32 / C251)
		ucState = EA;
    EA = 0; 
    
    u16High = g_u16OverflowCount;
    u16Low = ((uint16_t)T11H << 8) | T11L; // 读取硬件计数值
    
    // 再次检查溢出标志位（防止刚关中断前一瞬间溢出了但没进中断）
    // 如果硬件计数值很小（刚溢出），但溢出标志位还在，说明中断还没来得及处理
    // 注意：STC32 可能没有显式的 T2IF 标志位供查询，依赖中断自动处理
    // 如果无法查询溢出标志，上面的关中断逻辑通常足够应付 99.9% 的情况
    
    EA = ucState; // 开中断

    // 拼凑 32 位
    ulReturn = ((uint32_t)u16High << 16) | u16Low;
    
    return ulReturn;
}

char xdata pcWriteBuffer[512]={0};
void vTaskMonitor(void *pvParameters)
{
    // 分配一个足够大的缓冲区来存储统计信息
    // 每个任务大概需要 40 字节，如果有 10 个任务，建议分配 512 字节以上
    
		(void)pvParameters;
    while (1)
    {
        // 每 3 秒打印一次
        vTaskDelay(pdMS_TO_TICKS(3000));

        if (pcWriteBuffer != NULL)
        {
            // 获取统计信息
            // 格式为：任务名   运行时间(绝对值)   百分比
            vTaskGetRunTimeStats(pcWriteBuffer);

            // 通过串口打印出来
            Debug_Log("\r\n================ CPU Usage ================\r\n");
            Debug_Log("Task          Abs Time          %% Time\r\n");
            Debug_Log("-------------------------------------------\r\n");
            Debug_LogLongString(pcWriteBuffer);
            Debug_Log("===========================================\r\n");
        }
    }
}

void CPU_STATUS_TASK_Init(void){
    xTaskCreate(
        vTaskMonitor,
        "vTaskMonitor",
        256,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
}