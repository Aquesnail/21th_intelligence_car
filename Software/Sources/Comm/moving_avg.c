#include "moving_avg.h"

void ma_init(MovingAvg_t *ma, uint8_t window)
{
    uint8_t i;
    for (i = 0; i < MA_MAX_WINDOW; i++) {
        ma->buffer[i] = 0.0f;
    }
    ma->sum    = 0.0f;
    ma->window = window;
    ma->count  = 0;
    ma->head   = 0;
}

float ma_update(MovingAvg_t *ma, float input)
{
    uint8_t w;
    w = ma->window;
    if (w == 0) {
        return input;  /* 旁路 */
    }
    if (ma->count >= w) {
        /* 缓冲区已满, 移除最旧的样本 */
        ma->sum -= ma->buffer[ma->head];
    } else {
        ma->count++;
    }
    ma->buffer[ma->head] = input;
    ma->sum += input;
    ma->head++;
    if (ma->head >= w) {
        ma->head = 0;
    }
    return ma->sum / (float)(ma->count);
}

void ma_reset(MovingAvg_t *ma, float value)
{
    uint8_t i;
    uint8_t w;
    w = ma->window;
    if (w == 0) {
        return;
    }
    for (i = 0; i < w; i++) {
        ma->buffer[i] = value;
    }
    ma->sum   = value * (float)w;
    ma->count = w;
    ma->head  = 0;
}

void ma_set_window(MovingAvg_t *ma, uint8_t window)
{
    uint8_t w;
    uint8_t i;
    if (window > MA_MAX_WINDOW) {
        window = MA_MAX_WINDOW;
    }
    ma->window = window;
    w = window;
    if (w == 0) {
        ma->sum   = 0.0f;
        ma->count = 0;
        ma->head  = 0;
        return;
    }
    /* 重置缓冲区 */
    for (i = 0; i < w; i++) {
        ma->buffer[i] = 0.0f;
    }
    ma->sum   = 0.0f;
    ma->count = 0;
    ma->head  = 0;
}
