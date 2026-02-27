#ifndef _BEEP_H
#define _BEEP_H

#include <stdint.h>          /* uint16_t, uint8_t */
#include "config.h"          /* 假设包含项目配置 */
#include "simple_queue.h"     /* sQueue_t 等 */


/* 硬件蜂鸣器控制函数类型：freq=0 表示停止发声，duration 为持续时间（ms）*/
typedef void (*beep_out_t)(uint16_t freq);

#define BEEP_FULL 2
#define BEEP_ERR 1

typedef struct{
    uint16_t freq;
    uint16_t duration;
}BeepRequest_t;
/* 抽象定时器接口（由外部具体平台实现）*/
typedef struct {
    void *timer_ctx;                                    /* 定时器上下文句柄 */
    void (*start)(void *ctx, uint32_t timeout_ms,
                  void (*callback)(void*), void *arg); /* 启动定时器，到期调用 callback(arg) */
    void (*stop)(void *ctx);                            /* 停止定时器 */
} TimerInterface_t;
/* --- 新增：抽象同步接口（用于队列满时的阻塞与唤醒） --- */
typedef struct {
    void *sync_ctx;                                      /* 同步机制上下文句柄 */
    uint8_t (*wait)(void *ctx, uint32_t timeout_ms);     /* 等待可用空间，返回 1 成功，0 超时 */
    void (*notify)(void *ctx);                           /* 释放/通知可用空间 */
} BeepSyncInterface_t;

/* 内存分配器函数类型 */
typedef void* (*beep_malloc_t)(size_t size);
typedef void  (*beep_free_t)(void *ptr);

/* 蜂鸣器控制器不透明类型 */
typedef struct BeepController BeepController_t;

/**
 * 创建蜂鸣器控制器
 * @param hw_beep       硬件控制函数
 * @param timer_if      定时器接口（指针，内部会拷贝内容）
 * @param sync_if       信号量接口，传入NULL代表阻塞式调用
 * @param queue_capacity 队列最大容量（元素个数）
 * @param malloc_fn     外部内存分配函数
 * @param free_fn       外部内存释放函数
 * @return              控制器句柄，失败返回 NULL
 */
BeepController_t* BeepController_Create(beep_out_t hw_beep,
                                        const TimerInterface_t *timer_if,
                                        const BeepSyncInterface_t *sync_if, /* 新增参数 */
                                        uint16_t queue_capacity,
                                        beep_malloc_t malloc_fn,
                                        beep_free_t free_fn);

/**
 * 销毁蜂鸣器控制器，释放所有内存
 * @param ctrl 控制器句柄
 */
void BeepController_Destroy(BeepController_t *ctrl);

/**
 * 添加一个蜂鸣请求到播放队列
 * @param ctrl         控制器句柄
 * @param freq         频率（Hz），0 表示静音段
 * @param duration_ms  持续时间（ms）
 * @param timeout      若信号量满，最多挂起 ms
 * @return             0 成功，1 参数无效，2 队列满
 */
uint8_t BeepController_Play(BeepController_t *ctrl, uint16_t freq, uint16_t duration_ms,uint32_t timeout);

/**
 * 立即停止播放，清空队列
 * @param ctrl 控制器句柄
 */
void BeepController_Stop(BeepController_t *ctrl);

/**
 * 查询是否正在播放（队列非空或正在播放中）
 * @param ctrl 控制器句柄
 * @return     1 正在播放，0 空闲
 */
uint8_t BeepController_IsPlaying(const BeepController_t *ctrl);


#endif /* _BEEP_H */