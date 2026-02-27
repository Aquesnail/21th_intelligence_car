#include "beep.h"
#include <stddef.h> /* for NULL */

/* 蜂鸣器控制器内部结构体定义 */
struct BeepController {
    beep_out_t          hw_beep;      /* 硬件输出接口 */
    TimerInterface_t    timer_if;     /* 定时器接口 */
    BeepSyncInterface_t sync_if;      /* 同步接口 */
    sQueue_t            queue;        /* 播放队列 */
    void               *queue_buf;    /* 队列缓冲区内存指针 */
    beep_free_t         free_fn;      /* 内存释放函数 */
    
    uint8_t             is_busy;      /* 标志位：当前是否正在播放 */
    uint8_t             has_sync;     /* 是否启用了同步接口 */
};

/* 内部定时器回调函数：处理下一个播放请求 */
static void beep_process_next(void *arg) {
    BeepController_t *ctrl = (BeepController_t *)arg;
    BeepRequest_t req;

    /* 尝试从队列中取出一个请求 */
    if (sQueue_dequeue(&ctrl->queue, &req)) {
        /* 如果配置了同步接口，释放一个队列空间信号量 */
        if (ctrl->has_sync && ctrl->sync_if.notify) {
            ctrl->sync_if.notify(ctrl->sync_if.sync_ctx);
        }
        
        /* 设置硬件频率并发声 */
        ctrl->hw_beep(req.freq);
        
        /* 启动定时器，时间到了以后再次进入本回调函数 */
        ctrl->timer_if.start(ctrl->timer_if.timer_ctx, req.duration, beep_process_next, ctrl);
    } else {
        /* 队列为空，播放结束，关闭蜂鸣器 */
        ctrl->hw_beep(0);
        ctrl->is_busy = 0;
    }
}

BeepController_t* BeepController_Create(beep_out_t hw_beep,
                                        const TimerInterface_t *timer_if,
                                        const BeepSyncInterface_t *sync_if,
                                        uint16_t queue_capacity,
                                        beep_malloc_t malloc_fn,
                                        beep_free_t free_fn) 
{
		BeepController_t *ctrl;
    /* 基本参数校验 */
    if (!hw_beep || !timer_if || !malloc_fn || !free_fn || queue_capacity == 0) {
        return NULL;
    }

    /* 1. 分配控制器内存 */
    ctrl = (BeepController_t *)malloc_fn(sizeof(BeepController_t));
    if (!ctrl) {
        return NULL;
    }

    /* 2. 分配队列缓冲区内存 */
    ctrl->queue_buf = malloc_fn(queue_capacity * sizeof(BeepRequest_t));
    if (!ctrl->queue_buf) {
        free_fn(ctrl);
        return NULL;
    }

    /* 3. 初始化成员变量 */
    ctrl->hw_beep = hw_beep;
    ctrl->timer_if = *timer_if;
    ctrl->free_fn = free_fn;
    ctrl->is_busy = 0;

    if (sync_if) {
        ctrl->sync_if = *sync_if;
        ctrl->has_sync = 1;
    } else {
        ctrl->has_sync = 0;
    }

    /* 4. 初始化循环队列 */
    sQueue_init(&ctrl->queue, ctrl->queue_buf, queue_capacity, sizeof(BeepRequest_t));

    return ctrl;
}

void BeepController_Destroy(BeepController_t *ctrl) {
    if (!ctrl) return;
    
    /* 停止当前播放并清空状态 */
    BeepController_Stop(ctrl);
    
    /* 释放内存 */
    if (ctrl->queue_buf) {
        ctrl->free_fn(ctrl->queue_buf);
    }
    ctrl->free_fn(ctrl);
}

uint8_t BeepController_Play(BeepController_t *ctrl, uint16_t freq, uint16_t duration_ms, uint32_t timeout) {
    BeepRequest_t req;  /* 声明移至函数开头，遵循C89 */

    if (!ctrl) return BEEP_ERR;

    req.freq = freq;
    req.duration = duration_ms;

    /* 如果队列已满，根据是否提供同步接口决定是挂起等待还是直接返回失败 */
    if (sQueue_isFull(&ctrl->queue)) {
        if (ctrl->has_sync && ctrl->sync_if.wait) {
            /* 等待可用空间，若超时则返回队列满 */
            if (!ctrl->sync_if.wait(ctrl->sync_if.sync_ctx, timeout)) {
                return BEEP_FULL;
            }
        } else {
            /* 无同步接口（非阻塞）直接返回队列满 */
            return BEEP_FULL; 
        }
    }

    /* 将请求入队。这里正常情况下一定会成功，除非容量计算有误 */
    if (!sQueue_enqueue(&ctrl->queue, &req)) {
        return BEEP_FULL;
    }

    /* 如果当前处于空闲状态，手动启动第一次处理 */
    if (!ctrl->is_busy) {
        ctrl->is_busy = 1;
        beep_process_next(ctrl);
    }

    return 0;
}

void BeepController_Stop(BeepController_t *ctrl) {
    BeepRequest_t dummy;  /* 声明移至函数开头，遵循C89 */

    if (!ctrl) return;

    /* 停止定时器并静音硬件 */
    ctrl->timer_if.stop(ctrl->timer_if.timer_ctx);
    ctrl->hw_beep(0);
    ctrl->is_busy = 0;
    
    /* 清空队列并释放被占用的信号量（如果有） */
    while (sQueue_dequeue(&ctrl->queue, &dummy)) {
        if (ctrl->has_sync && ctrl->sync_if.notify) {
            ctrl->sync_if.notify(ctrl->sync_if.sync_ctx);
        }
    }
}

uint8_t BeepController_IsPlaying(const BeepController_t *ctrl) {
    if (!ctrl) return 0;
    return ctrl->is_busy;
}