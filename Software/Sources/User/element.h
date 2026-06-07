#ifndef ELEMENT_H
#define ELEMENT_H

#include "config.h"

typedef enum ElementType {
    ELEMENT_RING_LEFT,
    ELEMENT_RING_RIGHT,
    ELEMENT_SKIP,
    ELEMENT_BARREL,
    ELEMENT_WALL,
    ELEMENT_CROSS,
};

void Element_Init(void);

#endif // ELEMENT_H