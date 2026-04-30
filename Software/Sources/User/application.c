#include "application.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dma_uart.h"
#include "debug_arch.h"
#include "crc.h"
#include "storage_port.h"
#include "storage_mgr.h"
#include "beep.h"
#pragma warning disable = 47

extern void App_UartTest_Init(void);
extern void App_DebugTask_Init(void);
extern void APP_CLK_TEST_Init(void);
extern void CPU_STATUS_TASK_Init(void);
extern void IMU_Init();
extern void BeepPort_Init(void);
extern void BeepTestTask_Init();
extern void Task_OLED_Test_Init(void);
extern void key_port_init(void);
extern void MainControl_Init(void);
void app_init(void){
    UART4_Init();
    CRC_Init();
    //Storage_Init();
		//Params_Init_Default(); 
    //App_UartTest_Init();
		App_DebugTask_Init();
		APP_CLK_TEST_Init();
    CPU_STATUS_TASK_Init();
   // Beep_Init();
    IMU_Init();
   // MainControl_Init();
    
    //key_port_init();
   // Task_OLED_Test_Init();
    //Storage_Test_Init();
    vTaskStartScheduler();

}