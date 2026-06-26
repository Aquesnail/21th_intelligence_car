/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * Adapted for Keil C251 (C89) — 2024
 *   C251 keyword workaround: pdat = paged external RAM qualifier
 *   C251 has no native 64-bit integer
 ******************************************************************************
 */

#ifndef _VL53L1_TYPES_H_
#define _VL53L1_TYPES_H_

/*
 * ─── FIRST: include project types ───
 * config.h → DEF.H provides uint8_t, uint16_t, uint32_t etc.
 * Must come before any ST header that needs these types.
 */
#include "config.h"

/*
 * ─── C251 keyword workarounds ───
 * 'pdat' and 'data' are Keil C251 memory-space keywords.
 * ST driver headers use them as parameter/local-variable names
 * pervasively. These macros transparently rename every occurrence.
 *
 * Safe: the project uses LARGE memory model and never uses 'pdat'
 *       or 'data' as memory-space qualifiers.
 *       Struct fields use capital-D 'Data' (unaffected).
 */
#define pdat _pdat
#define data  _data

#include <stddef.h>
#include <string.h>

#ifndef NULL
#error "Error NULL definition should be done. Please add required include "
#endif

/*
 * C251 has no native 64-bit integer; alias to 32-bit.
 * The VL53L1X API's intermediate 64-bit calculations (histogram / dmax /
 * sigma) stay within 32-bit range for practical measurement scenarios
 * (max range ~4 m with mm precision).
 */
typedef uint32_t uint64_t;
typedef int32_t  int64_t;

/* Fixed-point 16.16 type */
typedef uint32_t FixPoint1616_t;

#endif /* _VL53L1_TYPES_H_ */
