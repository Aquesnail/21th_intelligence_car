/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * Adapted for Keil C251 (C89) — 2024
 *   C251 sizes: char=8, int=16, long=32, long long=64
 ******************************************************************************
 */

#ifndef _VL53L1_TYPES_H_
#define _VL53L1_TYPES_H_

#include <stddef.h>
#include <string.h>

#ifndef NULL
#error "Error NULL definition should be done. Please add required include "
#endif

/*
 * Standard integer types: project DEF.H usually provides these via config.h.
 * The fallback below covers the case when vl53l1_types.h is the first
 * stdint-style header included in a translation unit.
 */
#if !defined(_STDINT_H) && !defined(_STDINT_DEFINED) \
    && !defined(_GCC_STDINT_H) && !defined(__STDINT_DECLS) \
    && !defined(_GCC_WRAP_STDINT_H) && !defined(_STDINT)

typedef unsigned long long uint64_t;
typedef          long long int64_t;
typedef unsigned long      uint32_t;
typedef          long      int32_t;
typedef unsigned int       uint16_t;
typedef          int       int16_t;
typedef unsigned char      uint8_t;
typedef          char      int8_t;

#endif

/* Fixed-point 16.16 type */
typedef uint32_t FixPoint1616_t;

#endif /* _VL53L1_TYPES_H_ */
