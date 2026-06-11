#ifndef ELEMENT_H
#define ELEMENT_H

#include "config.h"

typedef enum ElementType {
    ELEMENT_STRAIGHT,
    ELEMENT_RING_LEFT_ENTRANCE,
    ELEMENT_RING_RIGHT_ENTRANCE,
    ELEMENT_RING_LEFT_PASS,       /* 左环岛开环通过中 */
    ELEMENT_RING_RIGHT_PASS,      /* 右环岛开环通过中 */
    ELEMENT_RING_LEFT_EXIT,
    ELEMENT_RING_RIGHT_EXIT,
    ELEMENT_SKIP,
    ELEMENT_BARREL,
    ELEMENT_WALL,
    ELEMENT_CROSS,
}element_t;

void Element_Init(void);
void Element_Handle(element_t *element, float adc_values[], float pitch, float roll, float yaw);
#endif // ELEMENT_H