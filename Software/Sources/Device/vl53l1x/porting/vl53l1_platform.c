/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * Complete rewrite for STC32G144K246 + FreeRTOS — 2024
 *
 * Maps VL53L1 platform API to:
 *   - I2C:  iic_user.h  (polling mode, P7.6=SCL / P7.7=SDA)
 *   - Tick: FreeRTOS xTaskGetTickCount() (1000 Hz → 1 tick = 1 ms)
 *   - GPIO: all stubs — XSHUT wired to 3V3, no interrupt pin connected
 ******************************************************************************
 */

/* ---- Project / platform headers ---- */
#include "config.h"               /* DEF.H, chip registers, MAIN_Fosc etc.     */
#include "delay.h"                /* delay_ms() busy-wait for pre-scheduler    */
#include "iic_user.h"             /* I2C1_ReadRegByte/Word, I2C1_WriteRegByte, */
                                  /* I2C1_ReadRegMulti, I2C1_WriteRegMulti     */
#include "FreeRTOS.h"
#include "task.h"                 /* vTaskDelay, xTaskGetTickCount              */

/* ---- VL53L1 driver headers ---- */
#include "vl53l1_platform.h"      /* API we implement                          */

/* ======================================================================== */
/*  Internal helpers                                                        */
/* ======================================================================== */

/*
 * Millisecond delay.
 * Uses FreeRTOS vTaskDelay when scheduler is running,
 * falls back to busy-wait delay_ms() otherwise.
 */
static void _port_delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        delay_ms((uint16_t)ms);
    }
}

/*
 * Microsecond delay — approximate busy-wait.
 * Calibrated for 120 MHz SYSCLK.  Accuracy is not critical;
 * the driver only uses us delays for power-up sequencing.
 */
static void _port_delay_us(uint32_t us)
{
    uint32_t i;
    while (us--) {
        /* ~ 1 μs at 120 MHz (tuned empirically, not cycle-accurate) */
        i = MAIN_Fosc / 8000000UL;
        while (--i) { /* nop */ }
    }
}

/*
 * Get current tick in milliseconds.
 * FreeRTOS tick = 1000 Hz → 1 tick = 1 ms.
 */
static uint32_t _port_tick_ms(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/* ======================================================================== */
/*  Comms (I2C) init / close                                                */
/* ======================================================================== */

VL53L1_Error VL53L1_CommsInitialise(
    VL53L1_DEV    Dev,
    uint8_t       comms_type,
    uint16_t      comms_speed_khz)
{
    (void)comms_type;
    (void)comms_speed_khz;

    /*
     * I2C hardware is already initialized by I2C1_Init()
     * (called from app layer before the VL53L1X task starts).
     * Nothing to do here except validate the handle.
     */
    if (Dev == NULL) {
        return VL53L1_ERROR_INVALID_PARAMS;
    }
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_CommsClose(VL53L1_DEV Dev)
{
    (void)Dev;
    return VL53L1_ERROR_NONE;
}

/* ======================================================================== */
/*  I2C multi-byte read / write                                             */
/* ======================================================================== */

VL53L1_Error VL53L1_WriteMulti(
    VL53L1_DEV    Dev,
    uint16_t      index,
    uint8_t      *pdata,
    uint32_t      count)
{
    if (Dev == NULL || pdata == NULL || count == 0 || count > 255) {
        return VL53L1_ERROR_INVALID_PARAMS;
    }

    I2C1_WriteRegMulti(Dev->i2c_wr_addr, index, pdata, (uint8_t)count);
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_ReadMulti(
    VL53L1_DEV    Dev,
    uint16_t      index,
    uint8_t      *pdata,
    uint32_t      count)
{
    if (Dev == NULL || pdata == NULL || count == 0 || count > 255) {
        return VL53L1_ERROR_INVALID_PARAMS;
    }

    I2C1_ReadRegMulti(Dev->i2c_wr_addr, index, pdata, (uint8_t)count);
    return VL53L1_ERROR_NONE;
}

/* ======================================================================== */
/*  I2C byte / word / dword                                                 */
/*  Data is big-endian on the wire (VL53L1 register map convention).        */
/* ======================================================================== */

VL53L1_Error VL53L1_WrByte(VL53L1_DEV Dev, uint16_t index, uint8_t dat)
{
    if (Dev == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    I2C1_WriteRegByte(Dev->i2c_wr_addr, index, dat);
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_RdByte(VL53L1_DEV Dev, uint16_t index, uint8_t *pdata)
{
    if (Dev == NULL || pdata == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    *pdata = I2C1_ReadRegByte(Dev->i2c_wr_addr, index);
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WrWord(VL53L1_DEV Dev, uint16_t index, uint16_t dat)
{
    uint8_t buf[2];
    if (Dev == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    buf[0] = (uint8_t)(dat >> 8);      /* MSB first (big-endian) */
    buf[1] = (uint8_t)(dat & 0xFF);
    I2C1_WriteRegMulti(Dev->i2c_wr_addr, index, buf, 2);
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_RdWord(VL53L1_DEV Dev, uint16_t index, uint16_t *pdata)
{
    uint8_t buf[2];
    if (Dev == NULL || pdata == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    I2C1_ReadRegMulti(Dev->i2c_wr_addr, index, buf, 2);
    *pdata = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WrDWord(VL53L1_DEV Dev, uint16_t index, uint32_t dat)
{
    uint8_t buf[4];
    if (Dev == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    buf[0] = (uint8_t)(dat >> 24);
    buf[1] = (uint8_t)(dat >> 16);
    buf[2] = (uint8_t)(dat >> 8);
    buf[3] = (uint8_t)(dat);
    I2C1_WriteRegMulti(Dev->i2c_wr_addr, index, buf, 4);
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_RdDWord(VL53L1_DEV Dev, uint16_t index, uint32_t *pdata)
{
    uint8_t buf[4];
    if (Dev == NULL || pdata == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    I2C1_ReadRegMulti(Dev->i2c_wr_addr, index, buf, 4);
    *pdata = ((uint32_t)buf[0] << 24)
           | ((uint32_t)buf[1] << 16)
           | ((uint32_t)buf[2] << 8)
           |  (uint32_t)buf[3];
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_UpdateByte(
    VL53L1_DEV Dev, uint16_t index, uint8_t AndData, uint8_t OrData)
{
    VL53L1_Error Status = VL53L1_ERROR_NONE;
    uint8_t data;

    Status = VL53L1_RdByte(Dev, index, &data);
    if (Status) return Status;

    data = (data & AndData) | OrData;
    return VL53L1_WrByte(Dev, index, data);
}

/* ======================================================================== */
/*  Timing                                                                  */
/* ======================================================================== */

VL53L1_Error VL53L1_WaitMs(VL53L1_DEV Dev, int32_t wait_ms)
{
    (void)Dev;
    if (wait_ms > 0) {
        _port_delay_ms((uint32_t)wait_ms);
    }
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitUs(VL53L1_DEV Dev, int32_t wait_us)
{
    (void)Dev;
    if (wait_us > 0) {
        _port_delay_us((uint32_t)wait_us);
    }
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GetTickCount(VL53L1_DEV Dev, uint32_t *ptime_ms)
{
    (void)Dev;
    if (ptime_ms == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    *ptime_ms = _port_tick_ms();
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GetTimerFrequency(int32_t *ptimer_freq_hz)
{
    if (ptimer_freq_hz == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    *ptimer_freq_hz = 0;    /* timestamps not used in this configuration */
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GetTimerValue(int32_t *ptimer_count)
{
    if (ptimer_count == NULL) return VL53L1_ERROR_INVALID_PARAMS;
    *ptimer_count = 0;
    return VL53L1_ERROR_NONE;
}

/* ======================================================================== */
/*  Register-poll helper: WaitValueMaskEx                                   */
/* ======================================================================== */

VL53L1_Error VL53L1_WaitValueMaskEx(
    VL53L1_DEV    Dev,
    uint32_t      timeout_ms,
    uint16_t      index,
    uint8_t       value,
    uint8_t       mask,
    uint32_t      poll_delay_ms)
{
    VL53L1_Error status         = VL53L1_ERROR_NONE;
    uint32_t     start_time_ms;
    uint32_t     current_time_ms;
    uint32_t     polling_time_ms;
    uint8_t      byte_value;
    uint8_t      found;

    start_time_ms  = _port_tick_ms();
    found          = 0;

    while ((status == VL53L1_ERROR_NONE)
        && (polling_time_ms < timeout_ms)
        && (found == 0)) {

        status = VL53L1_RdByte(Dev, index, &byte_value);

        if (status == VL53L1_ERROR_NONE
            && (byte_value & mask) == value) {
            found = 1;
        }

        if (status == VL53L1_ERROR_NONE
            && found == 0
            && poll_delay_ms > 0) {
            _port_delay_ms(poll_delay_ms);
        }

        current_time_ms = _port_tick_ms();
        polling_time_ms = current_time_ms - start_time_ms;
    }

    if (found == 0 && status == VL53L1_ERROR_NONE) {
        status = VL53L1_ERROR_TIME_OUT;
    }

    return status;
}

/* ======================================================================== */
/*  GPIO stubs                                                              */
/*                                                                          */
/*  XSHUT  → wired to 3V3 (always on)                                      */
/*  GPIO1  → not connected (polling mode, no interrupt)                     */
/*  NCS    → I2C mode selected by pin strapping                              */
/* ======================================================================== */

VL53L1_Error VL53L1_GpioSetMode(uint8_t pin, uint8_t mode)
{
    (void)pin; (void)mode;
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GpioSetValue(uint8_t pin, uint8_t value)
{
    (void)pin; (void)value;
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GpioGetValue(uint8_t pin, uint8_t *pvalue)
{
    (void)pin;
    if (pvalue != NULL) *pvalue = 0;
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GpioXshutdown(uint8_t value)
{
    (void)value;
    /* XSHUT pin is hard-wired to 3.3V — chip is always powered */
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GpioCommsSelect(uint8_t value)
{
    (void)value;
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GpioPowerEnable(uint8_t value)
{
    (void)value;
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GpioInterruptEnable(void (*function)(void), uint8_t edge_type)
{
    (void)function; (void)edge_type;
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_GpioInterruptDisable(void)
{
    return VL53L1_ERROR_NONE;
}
