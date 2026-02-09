#ifndef _VAR_TYPE_H
#define _VAR_TYPE_H

#include "config.h"

typedef enum {
    DBG_UINT8  = 0,
    DBG_INT8   = 1,
    DBG_UINT16 = 2,
    DBG_INT16  = 3,
    DBG_UINT32 = 4,
    DBG_INT32  = 5,
    DBG_FLOAT  = 6,
    DBG_STRING = 7,
    DBG_STATIC = 0x80
} VarType_t;



#endif