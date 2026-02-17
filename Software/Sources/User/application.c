#include "application.h"
#include "task.h"
#include "dma_uart.h"
#include "debug_arch.h"
#include "storage.h"
#include "crc.h"
#pragma warning disable = 47
extern void App_UartTest_Init(void);
extern void App_DebugTask_Init(void);
extern void APP_CLK_TEST_Init(void);
extern void Storage_Test_Init(void);
extern void CPU_STATUS_TASK_Init(void);
void app_init(void){
    UART5_Init(); 
    CRC_Init();
    Storage_Init();
    //App_UartTest_Init();
		App_DebugTask_Init();
		APP_CLK_TEST_Init();
    CPU_STATUS_TASK_Init();
    //Storage_Test_Init();
    vTaskStartScheduler();

}