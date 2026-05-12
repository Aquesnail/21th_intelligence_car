#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.h"
#include "adc_user.h"
#pragma warning disable = 98
#pragma warning disable = 91
extern void PWM_Test_Simple(void);
void app_clk_test(void)
{
    uint16_t hue;
    uint16_t breath_phase;
    uint8_t r, g, b;
    uint8_t breath;
    uint8_t h_val;

    hue = 0;
    breath_phase = 0;

    for(;;){
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

        WS2812_SetColor(0, r, g, b);
        WS2812_Show();

        hue++;
        breath_phase += 3;
        if(breath_phase >= 512) breath_phase = 0;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void APP_CLK_TEST_Init(void){
    xTaskCreate(app_clk_test, "AppClkTest", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}
