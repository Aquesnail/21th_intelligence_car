#include "params.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
// 引用 iap.c 中的函数声明 (或者包含 iap.h)
#include "iap.h"

// 全局参数实例
SystemParams_t g_params;
SemaphoreHandle_t xParamsMutex = NULL;
// ---------------------------------------------------------
// 内部常量：扇区大小 512 字节 
#define IAP_SECTOR_SIZE  512

// ---------------------------------------------------------
// 函数：将 Flash 中的数据全量加载到 RAM
// ---------------------------------------------------------
void Params_Load(void)
{
    // 读取很快，不需要关中断，只需要互斥锁保护 RAM 写入
    // (如果是上电初始化阶段调用，甚至锁都不需要，因为任务还没调度)
    uint16_t i;
    uint8_t *pBuf = (uint8_t *)&g_params;
    
    // 这里的 IAP_ReadByte 只是简单的 MOVC 指令，不涉及特殊时序
    for(i = 0; i < sizeof(SystemParams_t); i++) {
        pBuf[i] = IAP_ReadByte(PARAM_FLASH_ADDR_START + i);
    }
}
// ---------------------------------------------------------
// 函数：初始化 (上电调用)
// ---------------------------------------------------------
void Params_Init(void)
{
    // 1. 初始化 IAP 模块 (设置 IAP_TPS 等)
    // 注意：你需要在 IAP_Init 中确保 IAP_TPS 根据系统频率正确设置 [cite: 14] //神人STC，没有AI提醒我直接用的话又得调整半天，免费帮你这byd修bug
    IAP_Init(); 

    // 2. 加载参数
    Params_Load();
    xParamsMutex = xSemaphoreCreateMutex();
    // 注意：这里我们不再校验版本号，因为每次固件全量烧录都会重置 Flash
    // 我们信任 Flash 里的数据就是当前固件对应的初始值 (或上次保存的值)
}

// ---------------------------------------------------------
// 函数：将 RAM 数据全量保存到 Flash
// 注意：此函数耗时较长 (几毫秒到几十毫秒)，且会暂停 CPU
// 建议在电机停止或系统空闲时调用
// ---------------------------------------------------------
void Params_Save(void)
{
    uint16_t i;
    uint16_t sector_count;
    uint32_t sector_addr;
    uint8_t *pBuf = (uint8_t *)&g_params;
    uint16_t len = sizeof(SystemParams_t);
    
    // 1. 计算需要擦除多少个扇区
    // 公式：(总长度 + 511) / 512
    sector_count = (len + IAP_SECTOR_SIZE - 1) / IAP_SECTOR_SIZE;

    // 2. 逐个擦除扇区
    //  注意：必须先擦除才能写入
   if (xSemaphoreTake(xParamsMutex, portMAX_DELAY) == pdTRUE)
    {
        // 2. 进入临界区 (为了 IAP 硬件时序安全)
        taskENTER_CRITICAL(); 
        
        // --- 开始：系统假死区 (约30ms) ---

        // 擦除
        for(i = 0; i < sector_count; i++) {
             sector_addr = PARAM_FLASH_ADDR_START + (i * IAP_SECTOR_SIZE);
             IAP_EraseSector(sector_addr); // CPU在此处会有硬件级 stall
        }

        // 写入
        for(i = 0; i < len; i++) {
            IAP_ProgramByte(PARAM_FLASH_ADDR_START + i, pBuf[i]); // CPU在此处会有硬件级 stall
        }
        
        // --- 结束：系统恢复 ---

        taskEXIT_CRITICAL();
        
        // 3. 释放 Mutex
        xSemaphoreGive(xParamsMutex);
    }
    // 写入完成后，Flash 内的数据即为最新
}