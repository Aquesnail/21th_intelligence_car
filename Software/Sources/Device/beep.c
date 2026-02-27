#include "beep.h"
#include <string.h>          /* memcpy */

/* FreeRTOS 临界区保护（可替换为其他同步机制） */
#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h"
/* 控制器内部完整结构（对外隐藏） */
struct BeepController {
    beep_out_t          hw_beep;           /* 硬件控制函数 */
    TimerInterface_t    timer_if;          /* 定时器接口（副本） */
    BeepSyncInterface_t sync_if;
    sQueue_t            queue;             /* 循环队列 */
    uint8_t            *queue_buffer;      /* 动态分配的队列缓冲区 */
    uint8_t             is_playing;        /* 播放状态 */
    BeepRequest_t       current_req;       /* 当前正在播放的请求 */
    beep_malloc_t       malloc_fn;         /* 外部内存分配函数（仅供内部使用） */
    beep_free_t         free_fn;           /* 外部内存释放函数 */
};

/* 内部函数声明 */
static void BeepTimerCallback(void *arg);
static void StartNextRequest(BeepController_t *ctrl);

/*-----------------------------------------------------------*/
BeepController_t* BeepController_Create(beep_out_t hw_beep,
                                        const TimerInterface_t *timer_if,
                                        const BeepSyncInterface_t *sync_if,
                                        uint16_t queue_capacity,
                                        beep_malloc_t malloc_fn,
                                        beep_free_t free_fn)
{
    BeepController_t *ctrl;
    uint16_t buffer_size;

    if (malloc_fn == NULL || free_fn == NULL || timer_if == NULL) {
        return NULL;
    }

    /* 分配控制器本身 */
    ctrl = (BeepController_t*)malloc_fn(sizeof(BeepController_t));
    if (ctrl == NULL) {
        return NULL;
    }

    /* 分配队列缓冲区 */
    buffer_size = queue_capacity * sizeof(BeepRequest_t);
    ctrl->queue_buffer = (uint8_t*)malloc_fn(buffer_size);
    if (ctrl->queue_buffer == NULL) {
        free_fn(ctrl);
        return NULL;
    }

    /* 初始化队列 */
    sQueue_init(&ctrl->queue, ctrl->queue_buffer,
                queue_capacity, sizeof(BeepRequest_t));

    /* 保存外部依赖 */
    ctrl->hw_beep   = hw_beep;
    ctrl->timer_if  = *timer_if;   /* 拷贝接口结构体 */
    ctrl->malloc_fn = malloc_fn;
    ctrl->free_fn   = free_fn;
    
    if (sync_if != NULL) {
        ctrl->sync_if = *sync_if;
    } else {
        memset(&ctrl->sync_if, 0, sizeof(BeepSyncInterface_t));
    }

    ctrl->is_playing = 0;

    return ctrl;
}

/*-----------------------------------------------------------*/
void BeepController_Destroy(BeepController_t *ctrl)
{
    if (ctrl != NULL) {
        /* 停止定时器（如果有必要） */
        if (ctrl->timer_if.stop != NULL) {
            ctrl->timer_if.stop(ctrl->timer_if.timer_ctx);
        }
        /* 停止硬件发声 */
        if (ctrl->hw_beep) {
            ctrl->hw_beep(0);
        }
        /* 释放内存 */
        ctrl->free_fn(ctrl->queue_buffer);
        ctrl->free_fn(ctrl);
    }
}
uint8_t BeepController_Play(BeepController_t *ctrl, uint16_t freq, uint16_t duration_ms, uint32_t timeout_ms)
{
    BeepRequest_t req;
    uint8_t need_start = 0;

    if (ctrl == NULL) return BEEP_ERR;

    req.freq = freq;
    req.duration = duration_ms;

    /* 1. 先申请一个“空闲槽位” */
    if (ctrl->sync_if.wait != NULL) {
        /* 如果有同步接口，直接阻塞等待槽位。如果超时没拿到，说明队列真满了 */
        if (!ctrl->sync_if.wait(ctrl->sync_if.sync_ctx, timeout_ms)) {
            return BEEP_FULL; 
        }
    } else {
        /* 兼容裸机（未传入同步接口）的降级处理：手动检查队列是否已满 */
        taskENTER_CRITICAL();
        if (sQueue_isFull(&ctrl->queue)) {
            taskEXIT_CRITICAL();
            return BEEP_FULL;
        }
        taskEXIT_CRITICAL();
    }

    /* 2. 走到这里，说明【绝对有空位】！直接在临界区内无脑入队 */
    taskENTER_CRITICAL();
    sQueue_enqueue(&ctrl->queue, &req); /* 必定成功 */
    
    if (!ctrl->is_playing) {
        ctrl->is_playing = 1;
        need_start = 1;
    }
    taskEXIT_CRITICAL();

    /* 3. 如果原本是空闲状态，踹底层一脚让它开始干活 */
    if (need_start) {
        StartNextRequest(ctrl);
    }

    return 0;
}

static void StartNextRequest(BeepController_t *ctrl)
{
    BeepRequest_t req;
    uint8_t hasReq;

    taskENTER_CRITICAL();
    hasReq = sQueue_dequeue(&ctrl->queue, &req);
    if (!hasReq) {
        ctrl->is_playing = 0; 
    }
    taskEXIT_CRITICAL();

    if (hasReq) {
        /* --- 核心动作：成功拿走了一个音符，腾出了一个“空闲槽位”，归还给信号量 --- */
        if (ctrl->sync_if.notify != NULL) {
            ctrl->sync_if.notify(ctrl->sync_if.sync_ctx);
        }

        /* 正常发声并启动定时器... */
        ctrl->current_req = req;
        if (ctrl->hw_beep) {
            ctrl->hw_beep(req.freq == 0 ? 0 : req.freq);
        }
        if (ctrl->timer_if.start != NULL) {
            ctrl->timer_if.start(ctrl->timer_if.timer_ctx, req.duration, BeepTimerCallback, ctrl);
        }
    }
}
/*
蜂鸣器正在播放最后一曲，BeepTimerCallback 触发，调用 StartNextRequest。

StartNextRequest 在临界区内发现队列为空出队失败，退出临界区，准备执行 ctrl->is_playing = 0;。

此时发生任务调度（抢占），用户任务调用 BeepController_Play。

用户任务在临界区内成功入队，然后检查 if (!ctrl->is_playing)。由于定时器任务还没来得及把 is_playing 清零，这里读到的依然是 1。

用户任务认为蜂鸣器正在播放，跳过了 StartNextRequest()。

定时器任务恢复，将 is_playing 设为 0。
结果：新加入的蜂鸣请求永远被挂起，再也不会响了（直到下一次调用 Play 强制唤醒）。

所以修复时，我们需要把检查的操作和设置标志位都放在一个临界区里面就行了
*/


/*-----------------------------------------------------------*/
void BeepController_Stop(BeepController_t *ctrl)
{
    if (ctrl == NULL) return;

    if (ctrl->timer_if.stop != NULL) {
        ctrl->timer_if.stop(ctrl->timer_if.timer_ctx);
    }
    if (ctrl->hw_beep) {
        ctrl->hw_beep(0);
    }

    taskENTER_CRITICAL();
    /* 清空队列（直接重置索引） */
    ctrl->queue.front = 0;
    ctrl->queue.rear  = 0;
    taskEXIT_CRITICAL();

    ctrl->is_playing = 0;
}

/*-----------------------------------------------------------*/
uint8_t BeepController_IsPlaying(const BeepController_t *ctrl)
{
    return (ctrl != NULL) ? ctrl->is_playing : 0;
}

/*-----------------------------------------------------------*/
/* 定时器回调函数（由定时器接口在到期时调用）               */
static void BeepTimerCallback(void *arg)
{
    BeepController_t *ctrl = (BeepController_t*)arg;

    /* 停止硬件发声（确保静音，因为下一个请求可能也是空拍） */
    if (ctrl->hw_beep) {
        ctrl->hw_beep(0);
    }

    /* 处理下一个请求 */
    StartNextRequest(ctrl);
}