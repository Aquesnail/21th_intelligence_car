#ifndef ELEMENT_H
#define ELEMENT_H

#include "config.h"

typedef enum ElementType {
    ELEMENT_STRAIGHT,
    ELEMENT_RING_LEFT_ENTRANCE,
    ELEMENT_RING_RIGHT_ENTRANCE,
    ELEMENT_RING_LEFT_PASS,       /* 左环岛开环通过中 */
    ELEMENT_RING_RIGHT_PASS,      /* 右环岛开环通过中 */
    ELEMENT_IN_RING,              /* 环内行驶, 等待出环检测 */
    ELEMENT_RING_LEFT_EXIT,
    ELEMENT_RING_RIGHT_EXIT,
    ELEMENT_SKIP,
    ELEMENT_BARREL,
    ELEMENT_WALL,
    ELEMENT_CROSS,
    ELEMENT_SAFETY_STOP,          /* 安全停车: 翻车/离赛道超时 */
}element_t;

void Element_Init(void);
void Element_Handle(element_t *element, float adc_values[], float ax, float ay, float az, float yaw);
void Element_ResetOfftrackTimer(void);
void Element_ResetTrackIndex(void);  /* 重置赛道元素表索引, 供手动切自动/冲出赛道时调用 */
#endif // ELEMENT_H