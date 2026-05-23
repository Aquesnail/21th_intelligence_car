#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.h"
#include "adc_user.h"
#pragma warning disable = 98
#pragma warning disable = 91
#include "motor.h"
#include "config.h"

extern void PWM_Test_Simple(void);
extern volatile uint8_t g_p54_key_flag;
extern uint8_t manual_control_flag;
#define P54_LONG_PRESS_TICKS  40   /* 40 * 25ms = 1000ms */
extern void Change_Maunal();
void app_clk_test(void)
{
    uint16_t hue=0;
    uint16_t breath_phase=0;
    uint8_t r=0, g=0, b=0;
    uint8_t breath=0;
    uint8_t h_val=0;
    uint8_t  btn_pressed = 0;
    uint16_t btn_hold_cnt = 0;

    hue = 0;
    breath_phase = 0;

    for(;;){
        /* ---- P5.4 按键检测 ---- */
        if(g_p54_key_flag && !btn_pressed){
            g_p54_key_flag = 0;
            btn_pressed = 1;
            btn_hold_cnt = 0;
        }
        if(btn_pressed){
            if(!(P54 == 1)){       /* 按键仍在按下（低电平有效） */
                btn_hold_cnt++;
            } else {                   /* 按键已松开 */
                if(btn_hold_cnt >= P54_LONG_PRESS_TICKS){
                    /* 长按：复位 */
                    IAP_CONTR &= 0x40;
                    IAP_CONTR |= 0x20;
                } else {
                    /* 短按：切换手动模式 */
                    Change_Maunal();
                }
                btn_pressed = 0;
                btn_hold_cnt = 0;
            }
        }

        /* 三角波呼吸亮度: 0 -> 255 -> 0 */
        if(breath_phase < 256) {
            breath = (uint8_t)breath_phase;
        } else {
            breath = (uint8_t)(511 - breath_phase);
        }

        /* 色相转 RGB: 红 -> 黄 -> 绿 -> 青 -> 蓝 -> 紫 -> 红 */
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
        // MOTOR_CS_RESET();
        // motor_write_reg(0x03, (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b);
        // MOTOR_CS_SET();
        WS2812_SetColor(0, r, g, b);
        WS2812_Show();

        hue++;
        breath_phase += 3;
        if(breath_phase >= 512) breath_phase = 0;

        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

void APP_CLK_TEST_Init(void){
    xTaskCreate(app_clk_test, "AppClkTest", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}
