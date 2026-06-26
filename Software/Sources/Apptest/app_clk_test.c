#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.h"
#include "adc_user.h"
#include "debug_arch.h"
#include "iic_user.h"
#include <stdio.h>

#pragma warning disable = 98
#pragma warning disable = 91

/* VL53L1X I2C 地址 */
#define VL53L1X_I2C_ADDR     0x29
#define VL53L1X_WRITE_ADDR   (VL53L1X_I2C_ADDR << 1)       /* 0x52 */
#define VL53L1X_READ_ADDR    ((VL53L1X_I2C_ADDR << 1) | 1) /* 0x53 */

/* VL53L1X 识别寄存器 (16-bit 地址, 只读) */
#define VL53L1X_REG_MODEL_ID      0x010F   /* 预期值: 0xEA */
#define VL53L1X_REG_MODULE_TYPE   0x0110   /* 预期值: 0xCC */
#define VL53L1X_REG_MASK_REVISION 0x0111   /* 预期值: 0x10 */

void app_clk_test(void)
{
    uint8_t model_id;
    uint8_t module_type;
    uint8_t mask_revision;
    uint8_t led_on;
    char log_buf[128];

    led_on = 0;

    /* 任务启动标记 */
    WS2812_Clear();
    WS2812_SetColor(0, 0, 0, 50);  /* 蓝色 */
    WS2812_Show();
    vTaskDelay(pdMS_TO_TICKS(500));

    Debug_Log("VL53L1X Task Started\r\n");

    for (;;) {
        /* ---- 1. LED 闪烁 ---- */
        WS2812_Clear();
        if (led_on) {
            WS2812_SetColor(0, 50, 50, 0);  /* 黄色 */
        }
        WS2812_Show();
        led_on = !led_on;

        /* ---- 2. 读取 VL53L1X 识别寄存器 ---- */
        model_id      = I2C1_ReadRegByte(VL53L1X_WRITE_ADDR, VL53L1X_REG_MODEL_ID);
        module_type   = I2C1_ReadRegByte(VL53L1X_WRITE_ADDR, VL53L1X_REG_MODULE_TYPE);
        mask_revision = I2C1_ReadRegByte(VL53L1X_WRITE_ADDR, VL53L1X_REG_MASK_REVISION);

        /* ---- 3. 汇总打印 ---- */
        sprintf(log_buf,
            "VL53L1X: ModelID=0x%02X(%s) "
            "Module=0x%02X(%s) "
            "Mask=0x%02X(%s)\r\n",
            model_id,
            (model_id      == 0xEA) ? "OK" : "ERR",
            module_type,
            (module_type   == 0xCC) ? "OK" : "ERR",
            mask_revision,
            (mask_revision == 0x10) ? "OK" : "ERR");
        Debug_Log(log_buf);

        /* 500ms 周期 */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void APP_CLK_TEST_Init(void)
{
    I2C1_Init();
    xTaskCreate(app_clk_test, "AppClkTest", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}
