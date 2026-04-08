#include "storage_port.h"
#include "iap_user.h"
#include "imu_filter.h"
#include "storage_mgr.h"
#include "debug_arch.h"
#include <string.h>
#include <stdlib.h>

UserParams_t *UserParams;
extern Package_t g_ram_pkg;
extern IMU_Filter_t *IMU_Filter_Data;

/* 内部版本管理变量 */
static uint8_t g_curr_major = FIRMWARE_EXPECTED_MAJOR;
static uint8_t g_curr_minor = 0;

void Params_Link(void) {
    UserParams = &(g_ram_pkg.payload);
    IMU_Filter_Data = &(UserParams->imu_filter);
}



void Params_Init_Default(void) {
    
    Params_Link();
    /* 初始化时同步版本 */
    g_ram_pkg.major_ver = g_curr_major;
    g_ram_pkg.minor_ver = g_curr_minor;
    IMU_Filter_Data = &(UserParams->imu_filter);
    UserParams->imu_filter.Kp = 1.2;
    UserParams->imu_filter.Ki = 0.002;
    UserParams->imu_filter.bias_gx = 0.0;
    UserParams->imu_filter.bias_gy = 0.0;
    UserParams->imu_filter.bias_gz = 0.0;
    UserParams->imu_filter.is_calibrated = 0;
}


void Params_SetVersion(uint8_t major, uint8_t minor) {
    g_curr_major = major;
    g_curr_minor = minor;
    g_ram_pkg.major_ver = major;
    g_ram_pkg.minor_ver = minor;
}

uint8_t Storage_HandleCommand(const char* cmd) {
    /* C89: 变量必须在函数起始处声明 */
    const char* arg;
    
    if (strncmp(cmd, "stor ", 5) == 0) {
        arg = cmd + 5;
        
        if (strcmp(arg, "save") == 0) {
            Storage_Save();
            Debug_Log("Storage: OK, Saved.\r\n");
            return 1; 
        }
        else if (strcmp(arg, "read") == 0) {
            Debug_Log("Storage: OK, Read triggered.\r\n");
            return 1;
        }
        /* 上位机指令格式: "stor ver 01 05" (设置大版本1，小版本5) */
        else if (strncmp(arg, "ver ", 4) == 0) {
            uint8_t m_v, i_v;
            /* 简单的字符串解析 */
            m_v = (uint8_t)atoi(arg + 4);
            i_v = (uint8_t)atoi(arg + 7);
            Params_SetVersion(m_v, i_v);
            Debug_Log("Storage: Version Set OK.\r\n");
            return 1;
        }
        else {
            Debug_Log("Storage Err: try 'save', 'read', 'ver'\r\n");
            return 1; 
        }
    }
    return 0; 
}