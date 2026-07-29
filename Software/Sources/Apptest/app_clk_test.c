#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.h"
#include "led_indicator.h"
#include "adc_user.h"
#pragma warning disable = 98
#pragma warning disable = 91
#include "motor.h"
#include "config.h"

extern void PWM_Test_Simple(void);
extern volatile uint8_t g_p54_key_flag;
extern uint8_t manual_control_flag;
#define P54_LONG_PRESS_TICKS  500  /* 500 * 2ms = 1000ms */
extern void Change_Maunal();

/* 全局亮度缩放 (0-255), 值越小灯越暗. 255 = 原始亮度 */
#define LED_BRIGHTNESS  128

/* ================================================================
 * app_clk_test — LED 动画引擎任务 (2ms 周期 / 500Hz, 与主控同频)
 *
 *   双模式:
 *     MODE_BREATHING — 彩虹呼吸灯 (手动模式 / 安全停车时)
 *     MODE_STEADY    — 稳态单色灯 (自动循迹时, 颜色反映当前元素)
 *
 *   命令接口 (通过 volatile 全局变量, 由 app_main_control.c 写入):
 *     g_led_cmd = LED_CMD_STEADY          → 切换到稳态模式
 *     g_led_cmd = LED_CMD_RESUME_BREATHING → 恢复呼吸灯
 * ================================================================ */
void app_clk_test(void)
{
    /* 呼吸灯状态 */
    uint16_t hue;
    uint16_t breath_phase;
    uint8_t breath;
    uint8_t h_val;

    /* 稳态颜色 */
    uint8_t steady_r, steady_g, steady_b;

    /* RGB 输出 */
    uint8_t r, g, b;

    /* 当前模式 (本地缓存, 与 g_led_mode 同步) */
    uint8_t mode;

    /* 按键状态 */
    uint8_t  btn_pressed;
    uint16_t btn_hold_cnt;

    /* vTaskDelayUntil 精确周期控制 */
    TickType_t xLastWakeTime;

    /* 动画帧跳计数器 (降频保持原呼吸速度) */
    uint8_t anim_tick;

    /* ---- 初始化 ---- */
    hue = 0;
    breath_phase = 0;
    breath = 0;
    steady_r = 255;
    steady_g = 255;
    steady_b = 255;
    r = 0;
    g = 0;
    b = 0;
    mode = LED_MODE_BREATHING;
    g_led_mode = LED_MODE_BREATHING;
    btn_pressed = 0;
    btn_hold_cnt = 0;
    xLastWakeTime = xTaskGetTickCount();
    anim_tick = 0;

    for (;;) {

        /* ================================================================
         * 阶段1: 处理外部命令 (主控任务通过全局变量发送)
         * ================================================================ */
        if (g_led_cmd == LED_CMD_STEADY) {
            steady_r = g_led_r;
            steady_g = g_led_g;
            steady_b = g_led_b;
            mode = LED_MODE_STEADY;
            g_led_mode = LED_MODE_STEADY;
            g_led_cmd = LED_CMD_NONE;
        } else if (g_led_cmd == LED_CMD_RESUME_BREATHING) {
            mode = LED_MODE_BREATHING;
            g_led_mode = LED_MODE_BREATHING;
            g_led_cmd = LED_CMD_NONE;
        }

        /* ================================================================
         * 阶段2: 按键检测 (P5.4)
         *   短按: 切换手动/自动模式
         *   长按: 软件复位
         * ================================================================ */
        if (g_p54_key_flag && !btn_pressed) {
            g_p54_key_flag = 0;
            btn_pressed = 1;
            btn_hold_cnt = 0;
        }
        if (btn_pressed) {
            if (!(P54 == 1)) {       /* 按键仍在按下 (低电平有效) */
                btn_hold_cnt++;
            } else {                 /* 按键已松开 */
                if (btn_hold_cnt >= P54_LONG_PRESS_TICKS) {
                    /* 长按: 复位 */
                    IAP_CONTR &= 0x40;
                    IAP_CONTR |= 0x20;
                } else {
                    /* 短按: 切换手动模式 (Change_Maunal 内部会发 LED 命令) */
                    Change_Maunal();
                }
                btn_pressed = 0;
                btn_hold_cnt = 0;
            }
        }

        /* ================================================================
         * 阶段3: 执行当前 LED 模式
         * ================================================================ */
        if (mode == LED_MODE_BREATHING) {

            /* 三角波呼吸亮度: 0 → 255 → 0 */
            if (breath_phase < 256) {
                breath = (uint8_t)breath_phase;
            } else {
                breath = (uint8_t)(511 - breath_phase);
            }

            /* 色相转 RGB: 红→黄→绿→青→蓝→紫→红 */
            h_val = (uint8_t)hue;
            if      (h_val < 43)  { r = 255; g = h_val * 6; b = 0; }
            else if (h_val < 86)  { r = 255 - (h_val - 43) * 6;  g = 255; b = 0; }
            else if (h_val < 129) { r = 0;   g = 255; b = (h_val - 86) * 6; }
            else if (h_val < 172) { r = 0;   g = 255 - (h_val - 129) * 6; b = 255; }
            else if (h_val < 215) { r = (h_val - 172) * 6; g = 0; b = 255; }
            else                  { r = 255; g = 0; b = 255 - (h_val - 215) * 6; }

            /* 应用呼吸亮度 */
            r = (uint8_t)((uint16_t)r * breath / 255);
            g = (uint8_t)((uint16_t)g * breath / 255);
            b = (uint8_t)((uint16_t)b * breath / 255);

            anim_tick++;
            if (anim_tick >= 12) {
                anim_tick = 0;
                hue++;
                breath_phase += 3;
                if (breath_phase >= 512) breath_phase = 0;
            }

        } else {
            /* LED_MODE_STEADY: 稳态显示 */
            r = steady_r;
            g = steady_g;
            b = steady_b;
        }

        /* 全局亮度缩放 */
        r = (uint8_t)((uint16_t)r * LED_BRIGHTNESS / 255);
        g = (uint8_t)((uint16_t)g * LED_BRIGHTNESS / 255);
        b = (uint8_t)((uint16_t)b * LED_BRIGHTNESS / 255);

        /* 输出到 WS2812 */
        WS2812_SetColor(0, r, g, b);
        WS2812_Show();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2));
    }
}

void APP_CLK_TEST_Init(void)
{
    xTaskCreate(app_clk_test, "AppClkTest", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}
