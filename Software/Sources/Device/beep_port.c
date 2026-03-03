#include "beep_port.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "simple_queue.h"
#include <stddef.h>
#include "pwm.h"

/* =========================================================================
 * 宏定义与配置
 * ========================================================================= */
#define BEEP_MAX_INSTANCES         2 

/* 1. 底层控制器内部小队列 (直接对接定时器，无需太大，节省内存池) */
#define BEEP_CTRL_QUEUE_CAPACITY   8

/* 2. 外部异步缓冲大队列 (分配在 xdata 域，吸收业务层的海量数据) */
#define BEEP_ASYNC_QUEUE_CAPACITY  200

/* 3. 静态内存池大小 (仅用于 BeepController 结构体和小队列的分配) */
#define BEEP_POOL_SIZE             256

/* =========================================================================
 * 静态内存分配器 (用于控制器内部数据)
 * ========================================================================= */
static uint8_t beep_mem_pool[BEEP_POOL_SIZE];
static uint16_t beep_mem_offset = 0;

static void* port_beep_malloc(size_t len) {
    void *ptr = NULL;
    uint16_t align_offset;
    
    align_offset = (beep_mem_offset + 3) & ~3; /* 4字节对齐 */
    if ((align_offset + len) <= BEEP_POOL_SIZE) {
        ptr = &beep_mem_pool[align_offset];
        beep_mem_offset = align_offset + (uint16_t)len;
    }
    return ptr;
}

static void port_beep_free(void *ptr) {
    (void)ptr; /* 静态分配，不作回收 */
}

/* =========================================================================
 * 硬件底层实现
 * ========================================================================= */
static void hw_beep_0(uint16_t freq) {
    if(freq == 0){
        PWM_UpdateDuty(PWMA_CH3, 0);
        return;
    }
    PWMA_SetReload16(((100000/(freq)) - 1));
    PWM_UpdateDuty(PWMA_CH3,(uint16_t)(((100000/(freq)) - 1)/2)); 
}

static void hw_beep_1(uint16_t freq) {
    if(freq == 0){
        PWM_UpdateDuty(PWMA_CH4, 0);
        return;
    }
    PWMA_SetReload16(((100000/(freq)) - 1));
    PWM_UpdateDuty(PWMA_CH4, (uint16_t)(((100000/(freq)) - 1)/2)); 
}

static const beep_out_t hw_beep_funcs[BEEP_MAX_INSTANCES] = {
    hw_beep_0,
    hw_beep_1
};

/* =========================================================================
 * FreeRTOS 接口封装 (供底层 BeepController 使用)
 * ========================================================================= */
typedef struct {
    TimerHandle_t timer;
    void (*callback)(void*);
    void *arg;
} PortTimerCtx_t;

static void vBeepTimerCallback(TimerHandle_t xTimer) {
    PortTimerCtx_t *pCtx = (PortTimerCtx_t*)pvTimerGetTimerID(xTimer);
    if (pCtx && pCtx->callback) {
        pCtx->callback(pCtx->arg);
    }
}

static void port_timer_start(void *ctx, uint32_t timeout_ms, void (*callback)(void*), void *arg) {
    PortTimerCtx_t *pCtx = (PortTimerCtx_t*)ctx;
    TickType_t ticks;
    if (!pCtx) return;
    pCtx->callback = callback;
    pCtx->arg = arg;
    ticks = timeout_ms / portTICK_PERIOD_MS;
    if (ticks == 0) ticks = 1;
    xTimerChangePeriod(pCtx->timer, ticks, 0);
}

static void port_timer_stop(void *ctx) {
    PortTimerCtx_t *pCtx = (PortTimerCtx_t*)ctx;
    if (pCtx && pCtx->timer) {
        xTimerStop(pCtx->timer, 0);
    }
}

typedef struct {
    SemaphoreHandle_t sem;
} PortSyncCtx_t;

static uint8_t port_sync_wait(void *ctx, uint32_t timeout_ms) {
    PortSyncCtx_t *pCtx = (PortSyncCtx_t*)ctx;
    if (!pCtx || !pCtx->sem) return 0;
    return (xSemaphoreTake(pCtx->sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) ? 1 : 0;
}

static void port_sync_notify(void *ctx) {
    PortSyncCtx_t *pCtx = (PortSyncCtx_t*)ctx;
    if (pCtx && pCtx->sem) {
        xSemaphoreGive(pCtx->sem);
    }
}

/* =========================================================================
 * 全局控制器、上下文与异步缓冲 (大容量 xdata)
 * ========================================================================= */
static BeepController_t* g_beep_ctrls[BEEP_MAX_INSTANCES] = {NULL};
static PortTimerCtx_t      g_timer_ctxs[BEEP_MAX_INSTANCES];
static TimerInterface_t    g_timer_ifs[BEEP_MAX_INSTANCES];
static PortSyncCtx_t       g_sync_ctxs[BEEP_MAX_INSTANCES];
static BeepSyncInterface_t g_sync_ifs[BEEP_MAX_INSTANCES];

/* 独立任务与二级缓冲机制相关变量 */
static sQueue_t            g_async_queues[BEEP_MAX_INSTANCES];
static BeepRequest_t xdata g_async_bufs[BEEP_MAX_INSTANCES][BEEP_ASYNC_QUEUE_CAPACITY];
static SemaphoreHandle_t   g_async_wakeup_sem; /* 用于唤醒搬运任务的计数信号量 */

/* =========================================================================
 * 专门的数据搬运任务 (独立运行)
 * ========================================================================= */
static void vBeepDataHandlerTask(void *pvParameters) {
    uint8_t i;
    uint8_t has_data;
    BeepRequest_t req;

    (void)pvParameters;

    while (1) {
        /* 1. 阻塞等待：只要没有任何数据，任务就在此休眠，完全不占 CPU */
        xSemaphoreTake(g_async_wakeup_sem, portMAX_DELAY);

        /* 2. 被唤醒后，轮询所有实例的二级缓冲队列 */
        for (i = 0; i < BEEP_MAX_INSTANCES; i++) {
            
            /* 保护区：从大队列中取出一个音符 */
            taskENTER_CRITICAL();
            has_data = sQueue_dequeue(&g_async_queues[i], &req);
            taskEXIT_CRITICAL();

            if (has_data) {
                /* * 3. 推入底层控制器
                 * 注意：如果底层的小队列满了，这个调用会阻塞当前任务（portMAX_DELAY），
                 * 直到硬件定时器播完一个音符释放出空间。
                 * 这样就巧妙地把阻塞留在了后台任务，业务主任务绝对不会卡顿！
                 */
                BeepController_Play(g_beep_ctrls[i], req.freq, req.duration, portMAX_DELAY);
            }
        }
    }
}

/* =========================================================================
 * 模块初始化与对外接口
 * ========================================================================= */
void BeepPort_Init(void) {
    uint8_t i;

    /* 初始化用于唤醒搬运任务的信号量（最大计数值为所有队列容量总和） */
    g_async_wakeup_sem = xSemaphoreCreateCounting(BEEP_ASYNC_QUEUE_CAPACITY * BEEP_MAX_INSTANCES, 0);

    for (i = 0; i < BEEP_MAX_INSTANCES; i++) {
        /* 初始化底层接口 */
        g_timer_ctxs[i].timer = xTimerCreate("BeepTmr", 1, pdFALSE, &g_timer_ctxs[i], vBeepTimerCallback);
        g_timer_ifs[i].timer_ctx = &g_timer_ctxs[i];
        g_timer_ifs[i].start = port_timer_start;
        g_timer_ifs[i].stop = port_timer_stop;

        g_sync_ctxs[i].sem = xSemaphoreCreateCounting(BEEP_CTRL_QUEUE_CAPACITY, BEEP_CTRL_QUEUE_CAPACITY);
        g_sync_ifs[i].sync_ctx = &g_sync_ctxs[i];
        g_sync_ifs[i].wait = port_sync_wait;
        g_sync_ifs[i].notify = port_sync_notify;

        g_beep_ctrls[i] = BeepController_Create(
            hw_beep_funcs[i],
            &g_timer_ifs[i],
            &g_sync_ifs[i],
            BEEP_CTRL_QUEUE_CAPACITY,
            port_beep_malloc,
            port_beep_free
        );

        /* 初始化超大二级缓冲队列 */
        sQueue_init(&g_async_queues[i], g_async_bufs[i], BEEP_ASYNC_QUEUE_CAPACITY, sizeof(BeepRequest_t));
    }

    /* 创建独立的后台搬运任务 (优先级建议高于普通业务任务，低于核心控制任务) */
    xTaskCreate(vBeepDataHandlerTask, "BeepHandler", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
}

/**
 * 用户层对外接口：将音符压入大容量二级缓冲
 * 优势：业务层调用此函数几乎是 0 延迟，极速返回。
 */
void Beep_Out(uint8_t index, uint16_t freq, uint16_t time_ms) {
    BeepRequest_t req;
    uint8_t success;

    if (index >= BEEP_MAX_INSTANCES) return;

    req.freq = freq;
    req.duration = time_ms;

    /* 压入异步队列 (需关中断/临界区保护，防止多个任务同时调用导致队列数据错乱) */
    taskENTER_CRITICAL();
    success = sQueue_enqueue(&g_async_queues[index], &req);
    taskEXIT_CRITICAL();

    if (success) {
        /* 入队成功，释放一个信号量唤醒后台搬运任务 */
        xSemaphoreGive(g_async_wakeup_sem);
    }
}