/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * Adapted: custom device handle for STC32G platform.
 *          No BSP framework (vl53l1cb), direct I2C access via iic_user.h.
 ******************************************************************************
 */

#ifndef _VL53L1_PLATFORM_USER_DATA_H_
#define _VL53L1_PLATFORM_USER_DATA_H_

#include "vl53l1_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * VL53L1X device handle — minimalist struct.
 *
 * Contains the driver's internal LL data (LLDriverData + LLDriverResults,
 * together ~1440 bytes via VL53L1_DevData_t) plus the I2C write address.
 *
 * Place in XDATA to save internal DATA/IDATA RAM.
 */
typedef struct
{
    VL53L1_DevData_t  Data;        /* driver internals (~1440 bytes) */
    uint8_t           i2c_wr_addr; /* I2C write address = (7-bit addr << 1), default 0x52 */
} VL53L1X_Dev_t;

/* VL53L1_DEV ≡ pointer to our device struct */
typedef VL53L1X_Dev_t *VL53L1_DEV;

/* ---- Data-access macros (used pervasively inside modules/) ---- */
#define VL53L1DevDataGet(Obj, field)             ((Obj)->Data.field)
#define VL53L1DevDataSet(Obj, field, dat)        ((Obj)->Data.field = (dat))
#define PALDevDataGet(Obj, field)                ((Obj)->Data.field)
#define PALDevDataSet(Obj, field, value)         ((Obj)->Data.field = (value))
#define VL53L1DevStructGetLLDriverHandle(Obj)    (&(Obj)->Data.LLData)
#define VL53L1DevStructGetLLResultsHandle(Obj)   (&(Obj)->Data.llresults)

#ifdef __cplusplus
}
#endif

#endif /* _VL53L1_PLATFORM_USER_DATA_H_ */
