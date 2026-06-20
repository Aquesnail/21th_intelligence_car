#include "led_indicator.h"

/* ---- 共享变量定义 (主控写, LED任务读) ---- */
volatile uint8_t g_led_mode = LED_MODE_BREATHING;   /* 初始: 呼吸灯 */
volatile uint8_t g_led_cmd  = LED_CMD_NONE;
volatile uint8_t g_led_r    = 255;
volatile uint8_t g_led_g    = 255;
volatile uint8_t g_led_b    = 255;

/* ================================================================
 * LED_GetElementColor
 *   将元素类型映射为 RGB 颜色, 供 LED 任务稳态显示
 *   颜色语义:
 *     白  - 直道正常循迹
 *     蓝  - 进入环岛
 *     青  - 开环通过环岛
 *     黄  - 环内行驶
 *     绿  - 十字路口
 *     橙  - 上墙
 *     紫  - 过桶
 *     红  - 安全停车
 * ================================================================ */
void LED_GetElementColor(element_t element, uint8_t *r, uint8_t *g, uint8_t *b)
{
    const ElementProp_t *prop;

    prop = Element_GetProp(element);
    if (prop != NULL) {
        *r = prop->led_r;
        *g = prop->led_g;
        *b = prop->led_b;
    } else {
        /* 未找到 → 白色 (fallback) */
        *r = 255;
        *g = 255;
        *b = 255;
    }
}
