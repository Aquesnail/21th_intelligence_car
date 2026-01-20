#include "FreeRTOS.h"
#include "task.h"

void app_clk_test(void)
{
	for(;;){
    P20 = !P20;
		P21 = !P21;
    vTaskDelay(pdMS_TO_TICKS(5));
	}
}

void APP_CLK_TEST_Init(void){
    xTaskCreate(app_clk_test, "AppClkTest", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}