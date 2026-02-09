#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.h"
#pragma warning disable = 98
#pragma warning disable = 91
extern void PWM_Test_Simple(void);
void app_clk_test(void)
{
    uint8_t pos = 0;
	for(;;){
    // 1. 设置颜色
        WS2812_Clear(); // 先全黑
		vTaskDelay(pdMS_TO_TICKS(50));
        WS2812_SetColor(pos, 50, 50, 0); // 点亮第 pos 个灯，红色亮度50
        
        // 2. 发送
        WS2812_Show();
        
        // 3. 移动位置
        pos++;
        if(pos >= WS2812_LED_NUM) pos = 0;
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

void APP_CLK_TEST_Init(void){
    xTaskCreate(app_clk_test, "AppClkTest", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}