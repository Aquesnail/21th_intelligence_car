/* main.c - FreeRTOS 下使用蜂鸣器模块示例 */

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "beep.h"
#include "debug_arch.h"
#include "pwm.h"
#include "music.h"


#include <stdio.h>

uint8_t Beep_Out(uint8_t index,uint16_t freq,uint16_t ms);
void Beep_Init(uint8_t index);
void Beep_Delete(uint8_t index);

/* 硬件蜂鸣器控制函数（空实现，仅用于演示） */
static void hw_beep_1(uint16_t freq)
{
    (void)freq;
    if(freq == 0){
        PWM_UpdateDuty(PWMA_CH3,0);
        return;
    }
    PWMA_SetReload16(((10000/(freq)) - 1));
    PWM_UpdateDuty(PWMA_CH3, ((10000/(freq)) - 1)/2); 
    /* 实际项目中在此处操作硬件产生指定频率的方波，持续 duration ms */
}

/* ------------------------------------------------------------------
 * FreeRTOS 定时器适配层：实现 TimerInterface_t
 * ------------------------------------------------------------------ */

/* 定时器上下文结构（保存 FreeRTOS 定时器句柄及回调信息） */
typedef struct {
    TimerHandle_t timer;           /* FreeRTOS 定时器句柄 */
    void (*callback)(void*);       /* 用户到期回调函数 */
    void *arg;                      /* 回调函数参数 */
} FreeRTOS_TimerCtx;

typedef struct {
    SemaphoreHandle_t sem;
} FreeRTOS_SyncCtx;

/* FreeRTOS 定时器到期时的静态回调（由定时器服务任务调用） */
static void freertos_timer_callback(TimerHandle_t xTimer)
{
    FreeRTOS_TimerCtx *ctx = (FreeRTOS_TimerCtx *)pvTimerGetTimerID(xTimer); //传回创建它时传给定时器的上下文,实现多个定时器共用一个回调函数
    if (ctx && ctx->callback) {
        ctx->callback(ctx->arg);    /* 调用用户到期回调（即 BeepTimerCallback） */
    }
}

/* 发出通知（Give） */
static void freertos_sync_notify(void *ctx)
{
    FreeRTOS_SyncCtx *sync = (FreeRTOS_SyncCtx *)ctx;
    
    /* 注意：由于你的定时器是 FreeRTOS 软件定时器，其回调函数运行在 Timer Task 中，
       因此这里使用常规的 xSemaphoreGive 即可。
       如果你以后换成了硬件定时器中断，这里必须改成 xSemaphoreGiveFromISR！*/
    xSemaphoreGive(sync->sem);
}

/* 等待通知（Take） */
static uint8_t freertos_sync_wait(void *ctx, uint32_t timeout_ms)
{
    FreeRTOS_SyncCtx *sync = (FreeRTOS_SyncCtx *)ctx;
    TickType_t ticks = (timeout_ms == 0xFFFFFFFF) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    if (xSemaphoreTake(sync->sem, ticks) == pdTRUE) {
        return 1;
    }
    return 0;
}

/* 创建同步接口 */
static BeepSyncInterface_t freertos_sync_interface_create(uint16_t queue_capacity)
{
    BeepSyncInterface_t iface;
    FreeRTOS_SyncCtx *ctx = (FreeRTOS_SyncCtx *)pvPortMalloc(sizeof(FreeRTOS_SyncCtx));
    configASSERT(ctx != NULL);

    /* --- 核心变化：最大值为 capacity，初始值也为 capacity（表示一开始全是空槽位） --- */
    ctx->sem = xSemaphoreCreateCounting(queue_capacity, queue_capacity);
    configASSERT(ctx->sem != NULL);

    iface.sync_ctx = ctx;
    iface.wait = freertos_sync_wait;       /* 获取一个空槽位 */
    iface.notify = freertos_sync_notify;   /* 释放一个空槽位 */
    return iface;
}

static void freertos_sync_interface_destroy(BeepSyncInterface_t *iface)
{
    if (iface && iface->sync_ctx) {
        FreeRTOS_SyncCtx *ctx = (FreeRTOS_SyncCtx *)iface->sync_ctx;
        vSemaphoreDelete(ctx->sem);
        vPortFree(ctx);
        iface->sync_ctx = NULL;
    }
}

/* 启动定时器（实现 TimerInterface_t 的 start 函数） */
static void freertos_timer_start(void *ctx, uint32_t timeout_ms,
                                  void (*callback)(void*), void *arg)
{
    FreeRTOS_TimerCtx *timer_ctx = (FreeRTOS_TimerCtx *)ctx;
    BaseType_t ret;
    TickType_t timeout_ticks;
    /* 保存用户回调及参数 */
    timer_ctx->callback = callback;
    timer_ctx->arg = arg;

    /* 更改定时器周期（FreeRTOS 定时器以 tick 为单位，需转换） */
    timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    ret = xTimerChangePeriod(timer_ctx->timer, timeout_ticks, 0);
   // configASSERT(ret == pdPASS);

    /* 启动定时器（若已运行，xTimerChangePeriod 会保持运行状态） */
    if (xTimerIsTimerActive(timer_ctx->timer) == pdFALSE) {
        ret = xTimerStart(timer_ctx->timer, 0);
     //   configASSERT(ret == pdPASS);
    }
}

/* 停止定时器（实现 TimerInterface_t 的 stop 函数） */
static void freertos_timer_stop(void *ctx)
{
    FreeRTOS_TimerCtx *timer_ctx = (FreeRTOS_TimerCtx *)ctx;
    xTimerStop(timer_ctx->timer, 0);
}

/* 创建并初始化 FreeRTOS 定时器接口（返回 TimerInterface_t 结构体） */
static TimerInterface_t freertos_timer_interface_create(void)
{
    TimerInterface_t iface;
    FreeRTOS_TimerCtx *ctx;

    /* 分配定时器上下文 */
    ctx = (FreeRTOS_TimerCtx *)pvPortMalloc(sizeof(FreeRTOS_TimerCtx));
    configASSERT(ctx != NULL);

    /* 创建 FreeRTOS 软件定时器（自动重载为 pdFALSE，因为每次启动都需要新参数） */
    ctx->timer = xTimerCreate("BeepTimer",
                               portMAX_DELAY,        /* 初始周期很长，不会立即触发 */
                               pdFALSE,               /* 一次性定时器 */
                               ctx,                   /* 定时器 ID 指向上下文本身 */
                               freertos_timer_callback);
    configASSERT(ctx->timer != NULL);

    ctx->callback = NULL;
    ctx->arg = NULL;

    /* 填充接口函数 */
    iface.timer_ctx = ctx;
    iface.start = freertos_timer_start;
    iface.stop  = freertos_timer_stop;

    return iface;
}

/* 销毁定时器接口（释放资源） */
static void freertos_timer_interface_destroy(TimerInterface_t *iface)
{
    if (iface && iface->timer_ctx) {
        FreeRTOS_TimerCtx *ctx = (FreeRTOS_TimerCtx *)iface->timer_ctx;
        xTimerDelete(ctx->timer, 0);
        vPortFree(ctx);
        iface->timer_ctx = NULL;
    }
}
BeepController_t *beep_ctrl1;
TimerInterface_t timer1_if;
BeepController_t *beep_ctrl2;
TimerInterface_t timer2_if;
BeepSyncInterface_t sync1_if;

void vBeepTestTask(void *pvParameters){
    static MusicNote_t *song;
    static uint16_t index = 0;
    uint8_t ret;
		song  = song_two_tigers;
    while (1) {
        if (song[index].ms == 0) {
            Beep_Out(1, 0, 1000);
            break; 
        }

        /* 尝试入队。如果队列满，内部信号量会自动将当前任务挂起最多 2000ms。
           一旦定时器消耗了一个音符，任务立刻被唤醒，无需生硬的 vTaskDelay！ */
        ret = Beep_Out(1, song[index].freq, song[index].ms);

        if (ret == 0) {
            index++;
        } else if (ret == BEEP_FULL) {
            /* 2000ms 都没有空位，说明底层彻底罢工了，这里可以做错误处理 */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    vTaskDelete(NULL);
}

void Beep_Task_Init(){
    Beep_Init(1);
    xTaskCreate(
        vBeepTestTask,
        "BeepTest",
        256,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    return;
}


/* ------------------------------------------------------------------
 * 测试任务：演示蜂鸣器控制器的使用
 * ------------------------------------------------------------------ */
// void vBeepTestTask(void *pvParameters)
// {
   
//     uint8_t ret;

//     /* 1. 创建定时器接口 */
//     timer1_if = freertos_timer_interface_create(); //留下句柄用于销毁

//     /* 2. 创建蜂鸣器控制器，使用 FreeRTOS 的内存管理函数 */
//     beep_ctrl1 = BeepController_Create(hw_beep_1,
//                                       &timer1_if,
//                                       10,                     /* 队列容量 10 个请求 */
//                                       pvPortMalloc,           /* 内存分配函数 */
//                                       vPortFree);             /* 内存释放函数 */
//     configASSERT(beep_ctrl1 != NULL);

//     /* 3. 添加一些播放请求 */
//     Debug_Log("Adding beep requests...\n");

//     ret = BeepController_Play(beep_ctrl1, 1000, 500);  /* 1kHz，持续500ms */
//     configASSERT(ret == 0);
//     ret = BeepController_Play(beep_ctrl1, 2000, 300);  /* 2kHz，持续300ms */
//     configASSERT(ret == 0);
//     ret = BeepController_Play(beep_ctrl1, 0, 200);     /* 静音200ms（空拍） */
//     configASSERT(ret == 0);
//     ret = BeepController_Play(beep_ctrl1, 1500, 400);  /* 1.5kHz，持续400ms */
//     configASSERT(ret == 0);

//     Debug_Log("All requests added. Playing...\n");

//     /* 4. 等待一段时间，让蜂鸣器播放 */
//     vTaskDelay(pdMS_TO_TICKS(3000));

//     /* 5. 查询是否仍在播放 */
//     if (BeepController_IsPlaying(beep_ctrl1)) {
//         Debug_Log("Still playing after 3 seconds, stopping now.\n");
//         BeepController_Stop(beep_ctrl1);
//     } else {
//         Debug_Log("Playback finished.\n");
//     }

//     /* 6. 再添加一些请求，测试停止后的重新开始 */
//     Debug_Log("Adding two more requests...\n");
//     BeepController_Play(beep_ctrl1, 800, 600);
//     BeepController_Play(beep_ctrl1, 1200, 400);

//     vTaskDelay(pdMS_TO_TICKS(2000));

//     /* 7. 销毁控制器（会自动停止并释放内存） */
//     BeepController_Destroy(beep_ctrl1);

//     /* 8. 销毁定时器接口 */
//     freertos_timer_interface_destroy(&timer1_if);

//     Debug_Log("Test completed, deleting task.\n");
//     vTaskDelete(NULL);
// }


uint8_t Beep_Out(uint8_t index,uint16_t freq,uint16_t ms){
    if (index == 1) {
        return BeepController_Play(beep_ctrl1, freq, ms,3000);
    }
    return BEEP_ERR;
    
}

void Beep_Init(uint8_t index){
   if (index == 1) {
        timer1_if = freertos_timer_interface_create();
        sync1_if = freertos_sync_interface_create(10); /* 队列容量10 */

        beep_ctrl1 = BeepController_Create(hw_beep_1,
                                        &timer1_if,
                                        &sync1_if,     /* 传入同步接口 */
                                        10,            
                                        pvPortMalloc,  
                                        vPortFree);    
    }
}

void Beep_Delete(uint8_t index){
    switch (index)
    {
    case 1 :
        /*  销毁控制器（会自动停止并释放内存） */
        BeepController_Destroy(beep_ctrl1);

        /*  销毁定时器接口 */
        freertos_timer_interface_destroy(&timer1_if);
        break;
    
    default:
        break;
    }
}

// /* ------------------------------------------------------------------
//  * main() 函数：创建测试任务并启动调度器
//  * ------------------------------------------------------------------ */
// int main(void)
// {
//     /* 创建测试任务（优先级可根据需要调整） */
//     xTaskCreate(vBeepTestTask, "BeepTest", configMINIMAL_STACK_SIZE * 2,
//                 NULL, tskIDLE_PRIORITY + 1, NULL);

//     /* 启动 FreeRTOS 调度器 */
//     vTaskStartScheduler();

//     /* 正常情况下不会执行到这里 */
//     for (;;);
//     return 0;
// }