#include "debug_arch.h"
#include "storage.h"
#include "icm42688.h"
#include "imu_filter.h"

#define STORAGE_TEST_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)
#define STORAGE_TEST_TASK_STACK_SIZE    256

#pragma warning disable = 138

extern IMU_Filter_t IMU_Filter_Data;

void vStorageTestTask(void *pvParameters) {
    StorageErr_t err;
    uint8_t step = 0;
    
    (void)pvParameters;

    Debug_Log("[Storage] Init Done.\r\n");
    

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每5秒测试一次
       
    }
}

// ==========================================
// 任务创建函数 (供 main 调用)
// ==========================================
void Storage_APP_Init(void) {
    xTaskCreate(vStorageTestTask, 
                "StorTest", 
                STORAGE_TEST_TASK_STACK_SIZE, 
                NULL, 
                STORAGE_TEST_TASK_PRIORITY, 
                NULL);
}