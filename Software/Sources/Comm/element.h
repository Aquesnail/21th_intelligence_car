#ifndef ELEMENT_H
#define ELEMENT_H

#include "config.h"

typedef enum ElementType {
    ELEMENT_STRAIGHT,
    ELEMENT_RING_LEFT_ENTRANCE,
    ELEMENT_RING_RIGHT_ENTRANCE,
    ELEMENT_RING_LEFT_EXIT,
    ELEMENT_RING_RIGHT_EXIT,
    ELEMENT_SKIP,
    ELEMENT_BARREL,
    ELEMENT_WALL,
    ELEMENT_CROSS,
}element_t;

void Element_Init(void);

#endif // ELEMENT_H