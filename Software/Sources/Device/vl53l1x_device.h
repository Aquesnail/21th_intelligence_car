/**
 ******************************************************************************
 * vl53l1x_device.h — VL53L1X ToF 传感器设备封装
 *
 * 基于 ST VL53L1 API v6.6.18，提供精简的应用层接口。
 * 遵循项目 Device 层惯例（参考 icm42688.h）。
 ******************************************************************************
 */

#ifndef __VL53L1X_DEVICE_H__
#define __VL53L1X_DEVICE_H__

#include "config.h"

/* VL53L1X 默认 I2C 地址 */
#define VL53L1X_DEFAULT_ADDR        0x29    /* 7-bit address */

/* ---- 距离模式 ---- */
typedef enum {
    VL53L1X_DISTANCE_SHORT  = 1,    /* ~130 cm, best ambient immunity    */
    VL53L1X_DISTANCE_MEDIUM = 2,    /* ~300 cm                          */
    VL53L1X_DISTANCE_LONG   = 3     /* ~400 cm, lowest ambient immunity  */
} VL53L1X_DistanceMode;

/* ---- 测距结果 ---- */
typedef struct {
    uint16_t range_mm;              /* 距离 (毫米)                       */
    uint8_t  range_status;          /* 0 = valid, 非0 = 错误码           */
    uint16_t signal_rate_kcps;      /* 信号强度 (kcps)                   */
    uint16_t ambient_rate_kcps;     /* 环境光强度 (kcps)                 */
    uint16_t sigma_mm;              /* 测量不确定度 (毫米)               */
} VL53L1X_Result_t;

/* ---- 错误码 ---- */
#define VL53L1X_OK                  0
#define VL53L1X_ERROR              -1
#define VL53L1X_TIMEOUT            -2

/* ======================================================================== */
/*  API                                                                    */
/* ======================================================================== */

/*
 * 初始化 VL53L1X 传感器。
 * 调用前需先初始化 I2C1 (I2C1_Init)。
 *
 *   mode:            距离模式 (Short/Medium/Long)
 *   timing_budget_ms: 测量时序预算 (ms), 典型值 20-500ms
 *                     越大越精确，越小越省电。
 *                     设为 0 使用默认值 (Short=20, Medium=33, Long=50)
 *
 *   return: VL53L1X_OK 成功, VL53L1X_ERROR 失败
 */
int8_t VL53L1X_Init(VL53L1X_DistanceMode mode, uint32_t timing_budget_ms);

/*
 * 启动连续测距。
 * 调用 VL53L1X_Init 之后调用。
 *   return: VL53L1X_OK 成功
 */
int8_t VL53L1X_StartRanging(void);

/*
 * 获取一次测距结果 (非阻塞).
 * 如果数据尚未就绪，返回 VL53L1X_TIMEOUT。
 *
 *   result: 输出结果
 *   return: VL53L1X_OK / VL53L1X_TIMEOUT / VL53L1X_ERROR
 */
int8_t VL53L1X_GetResult(VL53L1X_Result_t *result);

/*
 * 停止连续测距。
 */
int8_t VL53L1X_StopRanging(void);

#endif /* __VL53L1X_DEVICE_H__ */
