#ifndef __LED_INDICATOR_H__
#define __LED_INDICATOR_H__

#include "element.h"

/* ---- LED 工作模式 ---- */
#define LED_MODE_BREATHING  0   /* 彩虹呼吸灯 (手动模式 / 安全停车) */
#define LED_MODE_STEADY     1   /* 稳态显示指定颜色 (自动循迹模式)   */

/* ---- 命令码 (主控写, LED任务读) ---- */
#define LED_CMD_NONE    0
#define LED_CMD_STEADY  1   /* 切换到稳态模式, 颜色见 g_led_r/g/b */
#define LED_CMD_RESUME_BREATHING  2   /* 恢复呼吸灯 */

/* ---- 共享变量 ---- */
extern volatile uint8_t g_led_mode;   /* 当前模式 (LED任务内部状态, 供外部查询) */
extern volatile uint8_t g_led_cmd;    /* 待执行命令 */
extern volatile uint8_t g_led_r;      /* 稳态目标: 红 */
extern volatile uint8_t g_led_g;      /* 稳态目标: 绿 */
extern volatile uint8_t g_led_b;      /* 稳态目标: 蓝 */

/* ---- API ---- */
void LED_GetElementColor(element_t element, uint8_t *r, uint8_t *g, uint8_t *b);

/* 便捷宏: 发命令给 LED 任务 */
#define LED_RequestSteady(r,g,b)       do { \
    g_led_r = (r); g_led_g = (g); g_led_b = (b); \
    g_led_cmd = LED_CMD_STEADY; \
} while(0)

#define LED_RequestBreathing()         do { \
    g_led_cmd = LED_CMD_RESUME_BREATHING; \
} while(0)

#endif /* __LED_INDICATOR_H__ */
