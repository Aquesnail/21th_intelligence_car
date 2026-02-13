#include "storage.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h" // 包含你的 Debug_Log 声明

// ==========================================
// 任务配置
// ==========================================
#define STORAGE_TEST_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)
#define STORAGE_TEST_TASK_STACK_SIZE    256

// ==========================================
// 测试变量 (必须是全局或静态，不能在栈上)
// ==========================================
// 也就是这些变量的地址会被注册进哈希表
static uint8_t xdata  test_cnt_u8  = 0;
static uint16_t xdata  test_cnt_u16 = 0;
static float   xdata  test_val_f   = 0.0f;
static uint32_t xdata test_big_u32 = 0;
static uint32_t u3222 = 12414;

// ==========================================
// 测试任务主体
// ==========================================
void vStorageTestTask(void *pvParameters) {
    StorageErr_t err;
    uint8_t step = 0;
    
    (void)pvParameters;

    // 1. 初始化存储模块
    Storage_Init();
    Debug_Log("[Storage] Init Done.\r\n");
    
    // 2. 注册变量
    // 注意：STC32 C251 的指针转换，这里显式强转 void far* 比较安全
    err = Storage_Register((void far*)&test_cnt_u8,  DBG_UINT8);
    if(err) Debug_Log("[Storage] Reg u8 Failed: %d\r\n", (int)err);

    err = Storage_Register((void far*)&test_cnt_u16, DBG_UINT16);
    if(err) Debug_Log("[Storage] Reg u16 Failed: %d\r\n", (int)err);

    err = Storage_Register((void far*)&test_val_f,   DBG_FLOAT);
    if(err) Debug_Log("[Storage] Reg float Failed: %d\r\n", (int)err);
    
    err = Storage_Register((void far*)&test_big_u32, DBG_UINT32);
    if(err) Debug_Log("[Storage] Reg u32 Failed: %d\r\n", (int)err);

    // 3. 上电初次读取 (Load)
    // 如果是第一次烧录，这里读出来的应该是 0xFF 或 0xFE (取决于你的默认值)
    Storage_Load((void far*)&test_cnt_u8);
    Storage_Load((void far*)&test_cnt_u16);
    Storage_Load((void far*)&test_val_f);
    Storage_Load((void far*)&test_big_u32);

    Debug_Log("[Storage] Loaded: u8=%u, u16=%u, f=%.2f\r\n", 
              (unsigned int)test_cnt_u8, test_cnt_u16, test_val_f);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每5秒测试一次
        step++;

        Debug_Log("\r\n--- [Storage Test Step %u] ---\r\n", (unsigned int)step);
     /*   
       Debug_Log("[Storage] DirectWrite %u \r\n",u3222);
       IAP_Write_4Bytes(0xFC000000,(void far*)u3222);
       Debug_Log("[Storage] Read %lu \r\n",u3222);
       IAP_EraseSector(0xFC000000 );
       IAP_Read_4Bytes(0xFC000000,(uint8_t far*)&u3222);
       Debug_Log("[Storage] Read %lu \r\n",u3222);*/
        // ==========================================
        // 步骤 A: 修改数据
        // ==========================================
        test_cnt_u8++;
        test_cnt_u16 = 100;
        test_val_f = 1.23f;
        test_big_u32 = 0x10000;

        Debug_Log("[RAM Modify] u8=%u, u16=%u, f=%.2f\r\n", 
                  (unsigned int)test_cnt_u8, test_cnt_u16, test_val_f);

        // ==========================================
        // 步骤 B: 标记并保存 (Write to Flash)
        // ==========================================
        // 标记所有变量为脏数据
        Storage_Mark_Data((void far*)&test_cnt_u8);
        Storage_Mark_Data((void far*)&test_cnt_u16);
        Storage_Mark_Data((void far*)&test_val_f);
        Storage_Mark_Data((void far*)&test_big_u32);

        // 执行扇区擦写
       // Debug_Log("[Storage] Saving...\r\n");
        Storage_Save_Data(); 
       // Debug_Log("[Storage] Save Done.\r\n");

        // ==========================================
        // 步骤 C: 验证测试 (关键！)
        // ==========================================
        // 1. 故意破坏 RAM 里的数据 (清零)
        test_cnt_u8 = 0;
        test_cnt_u16 = 0;
        test_val_f = 0.0f;
        Debug_Log("[Verify] RAM Cleared to 0. Loading from Flash...\r\n");

        // 2. 从 Flash 重新加载
        Storage_Load((void far*)&test_cnt_u8);
        Storage_Load((void far*)&test_cnt_u16);
        Storage_Load((void far*)&test_val_f);

        // 3. 检查数据是否恢复
        // 注意：test_cnt_u8 之前是++，如果恢复成功，它应该不是0
        Debug_Log("[Verify] Reload Result: u8=%u, u16=%u, f=%.2f\r\n", 
                  (unsigned int)test_cnt_u8, test_cnt_u16, test_val_f);
    }
}

// ==========================================
// 任务创建函数 (供 main 调用)
// ==========================================
void Storage_Test_Init(void) {
    xTaskCreate(vStorageTestTask, 
                "StorTest", 
                STORAGE_TEST_TASK_STACK_SIZE, 
                NULL, 
                STORAGE_TEST_TASK_PRIORITY, 
                NULL);
}