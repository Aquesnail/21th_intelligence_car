#ifndef MOVING_AVG_H
#define MOVING_AVG_H
#include "config.h"
/* ------------------------------------------------------------------
 * 移动平均滤波器 (O(1) 环形缓冲区)
 *
 *   window = 0: 旁路, 输出=输入
 *   window > 0: 最近 N 个样本的算术平均
 *
 *   最大窗口: 100
 * ------------------------------------------------------------------ */

#define MA_MAX_WINDOW 100

typedef struct {
    float   buffer[MA_MAX_WINDOW];
    float   sum;
    uint8_t window;   /* 窗口大小 0~100, 0=旁路 */
    uint8_t count;    /* 当前已存储的样本数 */
    uint8_t head;     /* 下一个写入位置 */
} MovingAvg_t;

/*
 * 初始化滤波器
 *   window: 窗口大小 (0~100), 0=旁路
 */
void ma_init(MovingAvg_t *ma, uint8_t window);

/*
 * 输入新样本, 返回移动平均值
 *   window==0 时直接返回 input (旁路)
 */
float ma_update(MovingAvg_t *ma, float input);

/*
 * 重置滤波器, 所有样本填充为 value
 *   window==0 时无操作
 */
void ma_reset(MovingAvg_t *ma, float value);

/*
 * 动态修改窗口大小 (会重置内部状态)
 *   window: 新窗口大小 (0~100), 超过 100 自动钳位
 */
void ma_set_window(MovingAvg_t *ma, uint8_t window);

#endif /* MOVING_AVG_H */
