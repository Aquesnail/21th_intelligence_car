#include "app_storage.h"
#include "debug_arch.h"
#include "storage.h"
#include "icm42688.h"
#include "imu_filter.h"

#define STORAGE_TEST_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)
#define STORAGE_TEST_TASK_STACK_SIZE    256

#pragma warning disable = 138

extern IMU_Filter_t IMU_Filter_Data;

SemaphoreHandle_t xDataMarkedSignal = NULL;
SemaphoreHandle_t xLoadAllData = NULL;

void Storage_Register_ALL(void){
    Storage_Register(&(IMU_Filter_Data.Kp),DBG_FLOAT,"IMU_Kp");
    Storage_Register(&(IMU_Filter_Data.Ki),DBG_FLOAT,"IMU_Ki");
    Storage_Register(&(IMU_Filter_Data.bias_gx),DBG_FLOAT,"bias_gx");
    Storage_Register(&(IMU_Filter_Data.bias_gy),DBG_FLOAT,"bias_gy");
    Storage_Register(&(IMU_Filter_Data.bias_gz),DBG_FLOAT,"bias_gz");
}

void Storage_Load_ALL(void){
    Storage_Load(&(IMU_Filter_Data.Kp));
    Storage_Load(&(IMU_Filter_Data.Ki));
    Storage_Load(&(IMU_Filter_Data.bias_gx));
    Storage_Load(&(IMU_Filter_Data.bias_gy));
    Storage_Load(&(IMU_Filter_Data.bias_gz));
}

void vStorageTask(void *pvParameters) {
    StorageErr_t err;
    uint8_t step = 0;
    
    (void)pvParameters;
    xDataMarkedSignal = xSemaphoreCreateBinary();
    xLoadAllData = xSemaphoreCreateBinary();
    Storage_Register_ALL();
    Debug_Log("[Storage] Init Done.\r\n");
    

    for (;;) {
        if((xSemaphoreTake(xDataMarkedSignal,portMAX_DELAY) == pdTRUE) || (xSemaphoreTake(xLoadAllData,portMAX_DELAY) == pdTRUE)){
            
            Storage_Save_Data();
            Storage_Load_ALL();
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每5秒测试一次  
    }
}

// ==========================================
// 任务创建函数 (供 main 调用)
// ==========================================
void Storage_APP_Init(void) {
    xTaskCreate(vStorageTask, 
                "StorTest", 
                STORAGE_TEST_TASK_STACK_SIZE, 
                NULL, 
                STORAGE_TEST_TASK_PRIORITY, 
                NULL);
}