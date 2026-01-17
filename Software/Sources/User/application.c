#include "application.h"
#include "task.h"
//#include "ws2812.h"
#include "params.h"
#pragma warning disable = 47
// 定义测试任务的句柄
TaskHandle_t xHandleTaskUART = NULL;

// 定义接收缓冲区 (放在xdata中节省内部RAM)
uint8_t xdata test_rx_buffer[64]; 

/**
 * @brief UART5 回显测试任务
 * * 功能：
 * 1. 启动时发送欢迎信息。
 * 2. 阻塞等待串口数据。
 * 3. 收到数据后，加上前缀 "Echo: " 发回。
 */
void vTaskUARTEcho(void *pvParameters)
{
		for(;;){

      vTaskDelay(5);
			P20=~P20;
    }
}
void app_init(void){
    /* 系统初始化 */
//    vSystemInit();
	//WS2812_Init_Driver();
	/* 创建任务 */
    xTaskCreate(vTaskUARTEcho, "UartEcho", 256, NULL, 1, &xHandleTaskUART);
    // xTaskCreate((TaskFunction_t )vUart2_5Task,
    //             (const char*    )"UART2_5",
    //             (uint16_t       )configDEFAULT_STACK_SIZE,
    //             (void*          )1000,  //注意pvParameters参数传地址或者立即数时只有低24位是有效位，最高8位编译时会自动填0. 可通过变量或者常量传送32字节数据.
    //             (UBaseType_t    )(configDEFAULT_PRIORITIES),
    //             (TaskHandle_t*  )NULL);
    //xTaskCreate(vTaskWS2812Test,"WS2812Test",512,NULL,1,NULL);
    /* 启动任务调度器开始任务调度 */
    vTaskStartScheduler();

}