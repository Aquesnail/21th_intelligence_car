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
    switch (element) {
        case ELEMENT_STRAIGHT:
            *r = 255; *g = 255; *b = 255;   /* 白: 直道 */
            break;
        case ELEMENT_RING_LEFT_ENTRANCE:
        case ELEMENT_RING_RIGHT_ENTRANCE:
            *r = 0;   *g = 0;   *b = 255;   /* 蓝: 进入环岛 */
            break;
        case ELEMENT_RING_LEFT_PASS:
        case ELEMENT_RING_RIGHT_PASS:
            *r = 0;   *g = 255; *b = 255;   /* 青: 开环通过 */
            break;
        case ELEMENT_IN_RING:
            *r = 255; *g = 255; *b = 0;     /* 黄: 环内 */
            break;
        case ELEMENT_CROSS:
            *r = 0;   *g = 255; *b = 0;     /* 绿: 十字 */
            break;
        case ELEMENT_WALL:
            *r = 255; *g = 128; *b = 0;     /* 橙: 上墙 */
            break;
        case ELEMENT_BARREL:
            *r = 255; *g = 0;   *b = 255;   /* 紫: 过桶 */
            break;
        case ELEMENT_SAFETY_STOP:
            *r = 255; *g = 0;   *b = 0;     /* 红: 安全停车 */
            break;
        default:
            *r = 255; *g = 255; *b = 255;   /* 未知: 默认白 */
            break;
    }
}
