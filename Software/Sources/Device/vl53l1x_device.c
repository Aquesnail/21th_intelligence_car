/**
 ******************************************************************************
 * vl53l1x_device.c — VL53L1X ToF 传感器设备封装实现
 *
 * 调用层次:
 *   App  → vl53l1x_device.c  → vl53l1_api.h (ST driver)
 *                              → vl53l1_platform.c (I2C / delay)
 *
 * 设备句柄 (VL53L1_DEV) 指向 XDATA 中的 VL53L1X_Dev_t,
 * 内部包含 ~1440 字节的 VL53L1_DevData_t。
 ******************************************************************************
 */

#include "vl53l1x_device.h"

/* ---- VL53L1 driver headers ---- */
#include "vl53l1_api.h"             /* VL53L1_WaitDeviceBooted, _DataInit, etc. */
#include "vl53l1_platform.h"        /* VL53L1_DEV, VL53L1_platform_user_data.h  */
#include "vl53l1_error_codes.h"     /* VL53L1_ERROR_NONE etc.                   */
#include "iic_user.h"               /* I2C1_Init (if not called yet)            */

/* ---- FreeRTOS ---- */
#include "FreeRTOS.h"
#include "task.h"

/* ======================================================================== */
/*  Device instance (in XDATA — ~1440 bytes)                               */
/* ======================================================================== */

static VL53L1X_Dev_t xdata g_dev;       /* device handle storage        */
static VL53L1_DEV     g_pDev = &g_dev;  /* pointer for ST driver API    */
static uint8_t        g_initialized = 0;/* init flag                    */
static uint8_t        g_ranging    = 0; /* ranging active flag          */

/* ======================================================================== */
/*  VL53L1X_Init                                                            */
/* ======================================================================== */

int8_t VL53L1X_Init(VL53L1X_DistanceMode mode, uint32_t timing_budget_ms)
{
    VL53L1_Error  status;
    uint16_t      default_budget;
    uint8_t       is_ready;
    VL53L1_DeviceInfo_t dev_info;
    uint8_t       model_id;

    /* -------- Step 0: sanity checks -------- */
    if (g_initialized) {
        return VL53L1X_OK;          /* already initialized */
    }

    /* I2C address */
    g_pDev->i2c_wr_addr = VL53L1X_DEFAULT_ADDR << 1;   /* 0x29 → 0x52 */

    /* -------- Step 1: verify device presence -------- */
    model_id = I2C1_ReadRegByte(g_pDev->i2c_wr_addr, 0x010F);
    if (model_id != 0xEA) {
        return VL53L1X_ERROR;       /* no VL53L1X on the bus */
    }

    /* -------- Step 2: wait for device boot -------- */
    status = VL53L1_WaitDeviceBooted(g_pDev);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    /* -------- Step 3: data init (reset internal state) -------- */
    status = VL53L1_DataInit(g_pDev);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    /* -------- Step 4: static init (NVM cal, SPAD, tuning parms) -------- */
    status = VL53L1_StaticInit(g_pDev);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    /* -------- Step 5: set preset mode -------- */
    status = VL53L1_SetPresetMode(g_pDev, VL53L1_PRESETMODE_AUTONOMOUS);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    /* -------- Step 6: set distance mode -------- */
    status = VL53L1_SetDistanceMode(g_pDev, (uint16_t)mode);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    /* -------- Step 7: set timing budget -------- */
    if (timing_budget_ms > 0) {
        status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(
                     g_pDev, timing_budget_ms * 1000UL);
    } else {
        /* Use driver default for selected mode */
        status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(
                     g_pDev, 33000UL);    /* 33 ms default */
    }
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    /* -------- Step 8: disable interrupt (we use polling) -------- */
    status = VL53L1_SetInterruptPolarity(g_pDev, 0);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    g_initialized = 1;
    return VL53L1X_OK;
}

/* ======================================================================== */
/*  VL53L1X_StartRanging                                                    */
/* ======================================================================== */

int8_t VL53L1X_StartRanging(void)
{
    VL53L1_Error status;

    if (!g_initialized) return VL53L1X_ERROR;

    status = VL53L1_StartMeasurement(g_pDev);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    g_ranging = 1;
    return VL53L1X_OK;
}

/* ======================================================================== */
/*  VL53L1X_GetResult (non-blocking)                                        */
/* ======================================================================== */

int8_t VL53L1X_GetResult(VL53L1X_Result_t *result)
{
    VL53L1_Error                    status;
    VL53L1_RangingMeasurementData_t range_data;
    uint8_t                         data_ready;

    if (!g_initialized || result == NULL) return VL53L1X_ERROR;

    /* Check if new data is available */
    status = VL53L1_GetMeasurementDataReady(g_pDev, &data_ready);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;
    if (data_ready == 0)            return VL53L1X_TIMEOUT;

    /* Read ranging result */
    status = VL53L1_GetRangingMeasurementData(g_pDev, &range_data);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    /* Clear the interrupt / data-ready flag */
    VL53L1_ClearInterruptAndStartMeasurement(g_pDev);

    /* Map to our simplified result type */
    result->range_mm          = range_data.RangeMilliMeter;
    result->range_status      = range_data.RangeStatus;
    result->signal_rate_kcps  = range_data.SignalRateRtnMegaCps;
    result->ambient_rate_kcps = range_data.AmbientRateRtnMegaCps;
    result->sigma_mm          = range_data.SigmaMilliMeter;

    return VL53L1X_OK;
}

/* ======================================================================== */
/*  VL53L1X_StopRanging                                                     */
/* ======================================================================== */

int8_t VL53L1X_StopRanging(void)
{
    VL53L1_Error status;

    if (!g_initialized || !g_ranging) return VL53L1X_OK;

    status = VL53L1_StopMeasurement(g_pDev);
    if (status != VL53L1_ERROR_NONE) return VL53L1X_ERROR;

    g_ranging = 0;
    return VL53L1X_OK;
}
