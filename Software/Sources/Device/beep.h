#ifndef _BEEP_H
#define _BEEP_H

#include <stdint.h>
#include "config.h"

typedef struct {
    uint16_t freq;      /* 频率Hz，0表示静音 */
    uint16_t duration;  /* 持续时间ms */
} BeepNote_t;

/* 初始化（系统启动时调用一次）*/
void Beep_Init(void);

/* 添加音符到队列，返回0成功，1队列满。非阻塞立即返回 */
uint8_t Beep_Play(uint16_t freq, uint16_t duration_ms);

/* 停止播放并清空队列 */
void Beep_Stop(void);

/* 是否正在播放 */
uint8_t Beep_IsBusy(void);

#endif