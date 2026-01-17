#ifndef __PARAMS_H__
#define __PARAMS_H__

#include "config.h" // 包含你的基础类型定义，如 uint8_t, uint32_t


// ================= 配置区域 =================

// 定义 EEPROM 起始地址，通常从 0 开始 (对应手册中的 IAP 地址 0x0000)
// 见手册 [cite: 184]
#define PARAM_FLASH_ADDR_START  0x0000 

// ================= 结构体定义 =================

// 必须使用 pack(1) 确保字节对齐，方便 PC 端解析
// (Keil C251 默认可能是 1 或 2 字节对齐，显式指定最安全)
#pragma pack(1) 
typedef struct {
    // 1. 协议头 (用于上位机识别)
    uint16_t version;     
    
    // 2. 电机参数
    float pid_p;
    float pid_i;
    float pid_d;
    float max_speed;
    
    // 3. 系统参数
    uint8_t  can_id;
    uint32_t baud_rate;
    uint8_t  device_name[16];

    // ... 你可以在这里无限添加参数，几百个字节都没问题 ...
    // ... 只要总大小不超过 EEPROM 容量 (16K) ...
    
} SystemParams_t;
#pragma pack() // 恢复默认对齐

// ================= 全局变量声明 =================

extern SystemParams_t g_params; // 全局参数实例

// ================= 函数声明 =================

void Params_Init(void);  // 上电初始化：读取参数
void Params_Save(void);  // 保存参数：写入 Flash

#endif
