#include "application.h"
#include "task.h"
//#include "ws2812.h"
#include "params.h"
#include "dma_uart.h"
#pragma warning disable = 47
extern void App_UartTest_Init(void);

void app_init(void){

    App_UartTest_Init();

    vTaskStartScheduler();

}