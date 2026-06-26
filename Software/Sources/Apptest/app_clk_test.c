#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.h"
#include "debug_arch.h"
#include "iic_user.h"

/* VL53L1X driver */
#include "vl53l1_api.h"
#include "vl53l1_platform_user_data.h"

#include <stdio.h>

#pragma warning disable = 98
#pragma warning disable = 91

/* ---- 设备句柄 (放在 XDATA, ~1440 bytes) ---- */
static VL53L1X_Dev_t xdata g_vl53l1_dev;

void app_clk_test(void)
{
    VL53L1_Error Status;
    VL53L1_RangingMeasurementData_t RangingData;
    uint8_t led_on = 0;
    uint8_t state;
    char log_buf[128];

    /* ---- 1. 启动标记 ---- */
    WS2812_Clear();
    WS2812_SetColor(0, 0, 0, 50);  /* 蓝色 = 初始化中 */
    WS2812_Show();

    Debug_Log("VL53L1X: Initializing...\r\n");

    /* ---- 2. 设置设备地址 ---- */
    g_vl53l1_dev.i2c_wr_addr = 0x29 << 1;  /* 7-bit → 8-bit write address = 0x52 */

    /* ---- 3. 标准初始化流程 ---- */
    Status = VL53L1_WaitDeviceBooted(&g_vl53l1_dev);
    if (Status) {
        sprintf(log_buf, "VL53L1X: WaitDeviceBooted FAILED (Status=%d)\r\n", (int)Status);
        Debug_Log(log_buf);
        goto init_error;
    }

    Status = VL53L1_DataInit(&g_vl53l1_dev);
    if (Status) {
        sprintf(log_buf, "VL53L1X: DataInit FAILED (Status=%d)\r\n", (int)Status);
        Debug_Log(log_buf);
        goto init_error;
    }

    Status = VL53L1_StaticInit(&g_vl53l1_dev);
    if (Status) {
        sprintf(log_buf, "VL53L1X: StaticInit FAILED (Status=%d)\r\n", (int)Status);
        Debug_Log(log_buf);
        goto init_error;
    }

    /* ---- 4. 配置测距参数 ---- */
    Status = VL53L1_SetPresetMode(&g_vl53l1_dev, VL53L1_PRESETMODE_LITE_RANGING);
    if (Status) {
        sprintf(log_buf, "VL53L1X: SetPresetMode FAILED (Status=%d)\r\n", (int)Status);
        Debug_Log(log_buf);
    }

    Status = VL53L1_SetDistanceMode(&g_vl53l1_dev, VL53L1_DISTANCEMODE_LONG);
    if (Status) {
        sprintf(log_buf, "VL53L1X: SetDistanceMode FAILED (Status=%d)\r\n", (int)Status);
        Debug_Log(log_buf);
    }

    /* 50 ms timing budget — 平衡速度与精度 */
    Status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(&g_vl53l1_dev, 50000);
    if (Status) {
        sprintf(log_buf, "VL53L1X: SetTimingBudget FAILED (Status=%d)\r\n", (int)Status);
        Debug_Log(log_buf);
    }

    /* ---- 5. 启动连续测距 ---- */
    Status = VL53L1_StartMeasurement(&g_vl53l1_dev);
    if (Status) {
        sprintf(log_buf, "VL53L1X: StartMeasurement FAILED (Status=%d)\r\n", (int)Status);
        Debug_Log(log_buf);
        goto init_error;
    }

    Debug_Log("VL53L1X: Init OK, ranging started.\r\n");
    WS2812_Clear();
    WS2812_SetColor(0, 0, 50, 0);  /* 绿色 = 就绪 */
    WS2812_Show();

    /* ---- 6. 主循环: 持续读取距离 ---- */
    for (;;) {
        /* 等待数据就绪 */
        Status = VL53L1_WaitMeasurementDataReady(&g_vl53l1_dev);
        if (Status) {
            sprintf(log_buf, "VL53L1X: WaitDataReady FAILED (Status=%d)\r\n", (int)Status);
            Debug_Log(log_buf);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* 获取测距数据 */
        Status = VL53L1_GetRangingMeasurementData(&g_vl53l1_dev, &RangingData);
        if (Status != VL53L1_ERROR_NONE) {
            sprintf(log_buf, "VL53L1X: GetRange FAILED (Status=%d)\r\n", (int)Status);
            Debug_Log(log_buf);
        } else {
            /* RangeStatus == 0 表示有效数据 */
            if (RangingData.RangeStatus == 0) {
                sprintf(log_buf,
                    "VL53L1X: Range=%d mm, "
                    "Signal=%.2f MCPS, "
                    "Ambient=%.2f MCPS, "
                    "SPAD=%d\r\n",
                    RangingData.RangeMilliMeter,
                    RangingData.SignalRateRtnMegaCps / 65536.0f,
                    RangingData.AmbientRateRtnMegaCps / 65536.0f,
                    RangingData.EffectiveSpadRtnCount);
            } else {
                sprintf(log_buf,
                    "VL53L1X: RangeStatus=%d (out of range / weak signal)\r\n",
                    RangingData.RangeStatus);
            }
            Debug_Log(log_buf);
        }

        /* 清除中断，触发下一次测量 */
        VL53L1_ClearInterruptAndStartMeasurement(&g_vl53l1_dev);

        /* LED 闪烁 */
        WS2812_Clear();
        if (led_on) {
            WS2812_SetColor(0, 30, 30, 0);  /* 黄 = 运行中 */
        } else {
            WS2812_SetColor(0, 0, 30, 0);    /* 绿 = 闲置 */
        }
        WS2812_Show();
        led_on = !led_on;

        /* ~100ms 周期 (50ms timing budget + 50ms 间隔) */
        vTaskDelay(pdMS_TO_TICKS(50));
    }

init_error:
    WS2812_Clear();
    WS2812_SetColor(0, 50, 0, 0);  /* 红色 = 失败 */
    WS2812_Show();
    Debug_Log("VL53L1X: Init FAILED — halting.\r\n");

    /* 获取内部状态辅助排查 */
    Status = VL53L1_GetPalState(&g_vl53l1_dev, &state);
    if (Status == VL53L1_ERROR_NONE) {
        sprintf(log_buf, "VL53L1X: internal state = %d\r\n", state);
        Debug_Log(log_buf);
    }

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void APP_CLK_TEST_Init(void)
{
    I2C1_Init();
    xTaskCreate(app_clk_test, "AppClkTest", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}
