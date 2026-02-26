#include "app_storage.h"
#include "event_groups.h"

#include "debug_arch.h"
#include "storage.h"
#include "icm42688.h"
#include "imu_filter.h"
#include "simple_queue.h"

#define STORAGE_TEST_TASK_PRIORITY      (tskIDLE_PRIORITY + 0)
#define STORAGE_TEST_TASK_STACK_SIZE    256


#define EVENT_SAVE_ALL (1<<0)
#define EVENT_MARK_ALL (1<<1)
#define EVENT_LOAD_ALL (1<<2)

#define EVENT_SINGLE_LOAD   (1<<3)
#define EVENT_SINGLE_MARK   (1<<4)


#pragma warning disable = 138

extern IMU_Filter_t IMU_Filter_Data;

sQueue_t queue_Marked_Data;
sQueue_t queue_Load_Data;
void* queue_Marked_Data_Buf[20];
void* queue_Load_Data_Buf[20];
SemaphoreHandle_t xStorageReadySignal = NULL;
EventGroupHandle_t xEventGroup;

void Storage_Register_ALL(void){
    Storage_Register(&(IMU_Filter_Data.Kp),4,"IMU_Kp");
    Storage_Register(&(IMU_Filter_Data.Ki),4,"IMU_Ki");
    Storage_Register(&(IMU_Filter_Data.bias_gx),4,"bias_gx");
    Storage_Register(&(IMU_Filter_Data.bias_gy),4,"bias_gy");
    Storage_Register(&(IMU_Filter_Data.bias_gz),4,"bias_gz");
}

void Storage_Load_ALL(void){
    Storage_Load(&(IMU_Filter_Data.Kp));
    Storage_Load(&(IMU_Filter_Data.Ki));
    Storage_Load(&(IMU_Filter_Data.bias_gx));
    Storage_Load(&(IMU_Filter_Data.bias_gy));
    Storage_Load(&(IMU_Filter_Data.bias_gz));
}

void Storage_Mark_ALL(void){
    Storage_Mark_Data(&(IMU_Filter_Data.Kp));
    Storage_Mark_Data(&(IMU_Filter_Data.Ki));
    Storage_Mark_Data(&(IMU_Filter_Data.bias_gx));
    Storage_Mark_Data(&(IMU_Filter_Data.bias_gy));
    Storage_Mark_Data(&(IMU_Filter_Data.bias_gz));
}

void Storage_Mark_Async(void* addr){
    sQueue_enqueue(&queue_Marked_Data,&addr);
    xEventGroupSetBits(xEventGroup,EVENT_SINGLE_MARK);
}

void Storage_Load_Async(void* addr){
    sQueue_enqueue(&queue_Load_Data,&addr);
    xEventGroupSetBits(xEventGroup,EVENT_SINGLE_LOAD);
}

void Storage_Save_Async(void){
    xEventGroupSetBits(xEventGroup,EVENT_SAVE_ALL);
}
void Storage_Load_All_Async(void){
    xEventGroupSetBits(xEventGroup,EVENT_LOAD_ALL);
}
void Storage_Mark_All_Async(void){
    xEventGroupSetBits(xEventGroup,EVENT_MARK_ALL);
}

void vStorageTask(void *pvParameters) {
    StorageErr_t err;
    uint8_t step = 0;
    EventBits_t uxBits;
    (void)pvParameters;
    (void)err;
    Storage_Register_ALL();
    
    if(xDebugReadySignal != NULL){
        xSemaphoreTake(xDebugReadySignal,portMAX_DELAY);
        xSemaphoreGive(xDebugReadySignal);
    }
    Debug_Log("[Storage] Init Done.\r\n");

    sQueue_init(&queue_Load_Data, queue_Load_Data_Buf, 20, sizeof(void*));
    sQueue_init(&queue_Marked_Data, queue_Marked_Data_Buf, 20, sizeof(void*));
    xSemaphoreGive(xStorageReadySignal);
    for (;;) {
        // 等待所有存储相关事件
        uxBits = xEventGroupWaitBits(
            xEventGroup,
            EVENT_SAVE_ALL | EVENT_MARK_ALL | EVENT_LOAD_ALL | EVENT_SINGLE_LOAD | EVENT_SINGLE_MARK,
            pdTRUE,        // 退出时清除事件位
            pdFALSE,       // 不需要所有位同时满足
            portMAX_DELAY  // 一直等待
        );

        // 处理批量保存
        if (uxBits & EVENT_SAVE_ALL) {
            Storage_Save_Data();
            Debug_Log("Data Saved\r\n");
        }

        // 处理批量标记
        if (uxBits & EVENT_MARK_ALL) {
            Storage_Mark_ALL();
            Debug_Log("Data Marked All\r\n");
        }

        // 处理批量加载
        if (uxBits & EVENT_LOAD_ALL) {
            Storage_Load_ALL();
            Debug_Log("Data_Loaded_All\r\n");
        }

        // 处理单次加载（从队列中取出所有待加载地址）
        if (uxBits & EVENT_SINGLE_LOAD) {
            void* addr;
            while (sQueue_dequeue(&queue_Load_Data, &addr)) {
                Storage_Load(addr);
            }
        }

        // 处理单次标记（从队列中取出所有待标记地址）
        if (uxBits & EVENT_SINGLE_MARK) {
            void* addr;
            while (sQueue_dequeue(&queue_Marked_Data, &addr)) {
                Storage_Mark_Data(addr);
            }
        }
    }
}

// ==========================================
// 任务创建函数 (供 main 调用)
// ==========================================
void Storage_App_Init(void) {
    xEventGroup = xEventGroupCreate();
    xStorageReadySignal = xSemaphoreCreateBinary();
    xTaskCreate(vStorageTask, 
                "StorTest", 
                STORAGE_TEST_TASK_STACK_SIZE, 
                NULL, 
                STORAGE_TEST_TASK_PRIORITY, 
                NULL);
}

uint8_t Storage_HandleCommand(const char* cmd) {
    if (strcmp(cmd, "save") == 0) {
        xEventGroupSetBits(xEventGroup, EVENT_SAVE_ALL);
        Debug_Log("Storage: SAVE_ALL triggered\r\n");
        return 1;
    } else if (strcmp(cmd, "markall") == 0) {
        xEventGroupSetBits(xEventGroup, EVENT_MARK_ALL);
        Debug_Log("Storage: MARK_ALL triggered\r\n");
        return 1;
    } else if (strcmp(cmd, "loadall") == 0) {
        xEventGroupSetBits(xEventGroup, EVENT_LOAD_ALL);
        Debug_Log("Storage: LOAD_ALL triggered\r\n");
        return 1;
    }
    return 0; // 未识别
}