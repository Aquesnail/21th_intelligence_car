/**
 ******************************************************************************
 * app_vl53l1x.c — VL53L1X ToF 传感器测试任务
 *
 * 功能:
 *   1. 初始化 I2C1 + VL53L1X, 设置 Medium 距离模式
 *   2. 启动连续测距
 *   3. 每 100ms 读取一次距离, 通过 Debug_Log 输出
 *   4. WS2812 LED 显示测距状态
 *
 * 添加到 application.c:
 *   extern void APP_VL53L1X_Init(void);
 *   在 app_init() 中调用 APP_VL53L1X_Init();
 ******************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h"         /* Debug_Log */
#include "ws2812.h"             /* WS2812 LED */
#include "iic_user.h"           /* I2C1_Init */
#include "vl53l1x_device.h"     /* VL53L1X_* */
#include <stdio.h>              /* sprintf */

#pragma warning disable = 98
#pragma warning disable = 91

/* ---- 任务配置 ---- */
#define VL53L1X_TASK_STACK      1024
#define VL53L1X_TASK_PRIORITY   (tskIDLE_PRIORITY + 2)

/* ---- 测距参数 ---- */
#define VL53L1X_MODE            VL53L1X_DISTANCE_MEDIUM
#define VL53L1X_BUDGET_MS       50      /* 50ms 时序预算 */
#define VL53L1X_READ_PERIOD_MS  100     /* 100ms 读取周期 */

/* ---- 任务函数 ---- */
void app_vl53l1x_task(void *pvParameters)
{
    VL53L1X_Result_t result;
    int8_t           rc;
    char             log_buf[128];
    uint8_t          led_on;
    uint32_t         distance_mm;

    (void)pvParameters;
    led_on = 0;

    /* ===== 启动信号 ===== */
    WS2812_Clear();
    WS2812_SetColor(0, 0, 50, 0);   /* 绿色: 启动 */
    WS2812_Show();
    vTaskDelay(pdMS_TO_TICKS(500));

    Debug_Log("[VL53L1X] Task Started\r\n");

    /* ===== Step 1: 初始化传感器 ===== */
    Debug_Log("[VL53L1X] Initializing...\r\n");
    rc = VL53L1X_Init(VL53L1X_MODE, VL53L1X_BUDGET_MS);
    if (rc != VL53L1X_OK) {
        Debug_Log("[VL53L1X] *** INIT FAILED ***\r\n");
        /* 错误指示: 红色闪烁 */
        for (;;) {
            WS2812_Clear();
            WS2812_SetColor(0, 50, 0, 0);  /* 红色: 初始化失败 */
            WS2812_Show();
            vTaskDelay(pdMS_TO_TICKS(500));
            WS2812_Clear();
            WS2812_Show();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    Debug_Log("[VL53L1X] Init OK\r\n");

    /* ===== Step 2: 启动连续测距 ===== */
    rc = VL53L1X_StartRanging();
    if (rc != VL53L1X_OK) {
        Debug_Log("[VL53L1X] *** START FAILED ***\r\n");
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    Debug_Log("[VL53L1X] Ranging started\r\n");

    /* ===== Step 3: 主循环 ===== */
    for (;;) {
        /* 读距离 */
        rc = VL53L1X_GetResult(&result);

        /* LED 状态 */
        WS2812_Clear();
        if (rc == VL53L1X_OK) {
            distance_mm = result.range_mm;
            if (result.range_status == 0 && distance_mm < 4000) {
                /* 有效距离 → 绿色, 亮度与距离成反比 */
                uint8_t brightness = (uint8_t)(50 - (distance_mm * 12 / 1000));
                WS2812_SetColor(0, 0, brightness, 0);
            } else {
                /* 超出量程 → 黄色 */
                WS2812_SetColor(0, 30, 30, 0);
            }
        } else if (rc == VL53L1X_TIMEOUT) {
            /* 数据未就绪 → 蓝色 */
            WS2812_SetColor(0, 0, 0, 20);
        } else {
            /* 错误 → 红色 */
            WS2812_SetColor(0, 30, 0, 0);
        }
        WS2812_Show();

        /* 日志输出 */
        if (rc == VL53L1X_OK) {
            sprintf(log_buf,
                "[VL53L1X] %4lu mm | sigma=%2u | sig=%3u | amb=%3u | status=%u\r\n",
                (unsigned long)distance_mm,
                (unsigned int)result.sigma_mm,
                (unsigned int)result.signal_rate_kcps,
                (unsigned int)result.ambient_rate_kcps,
                (unsigned int)result.range_status);
            Debug_Log(log_buf);
        } else if (rc == VL53L1X_TIMEOUT) {
            /* 不打印, 避免刷屏 */
        } else {
            sprintf(log_buf, "[VL53L1X] ERROR rc=%d\r\n", (int)rc);
            Debug_Log(log_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(VL53L1X_READ_PERIOD_MS));
    }
}

/* ===== 初始化入口 (由 application.c 调用) ===== */
void APP_VL53L1X_Init(void)
{
    I2C1_Init();    /* 初始化 I2C1 (P7.6=SCL, P7.7=SDA) */
    xTaskCreate(app_vl53l1x_task, "VL53L1X",
                VL53L1X_TASK_STACK, NULL,
                VL53L1X_TASK_PRIORITY, NULL);
}
