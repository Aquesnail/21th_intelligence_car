#include "storage_port.h"
#include "iap_user.h"

#include "imu_filter.h"
#include "storage_mgr.h"
#include "debug_arch.h"
UserParams_t *UserParams;
extern Package_t g_ram_pkg;

extern IMU_Filter_t *IMU_Filter_Data;

void Params_Init_Default(){
    IMU_Filter_Data = &(UserParams->imu_filter);  // ✅ 提前初始化指针,我是傻逼
    UserParams->imu_filter.Kp = 1.2;
    UserParams->imu_filter.Ki = 0.002;
    UserParams->imu_filter.bias_gx = 0.0;
    UserParams->imu_filter.bias_gy = 0.0;
    UserParams->imu_filter.bias_gz = 0.0;

    //Storage_Save();
}
void Params_Link(){
    UserParams = &(g_ram_pkg.payload);
}
void Params_Load(){
    /* 加载逻辑已在 Storage_Init() 中完成，
       此函数保留用于未来扩展（如运行时重新加载） */
}

/* * 极简版 Storage 命令行处理
 * 仅支持基础触发指令：
 * "storage save"  - 触发保存
 * "storage read"  - 触发读取/打印状态
 * "storage reset" - 触发重置/清空
 */
uint8_t Storage_HandleCommand(const char* cmd) {
    // 1. 检查前缀，看看是不是找我们 storage 模块的命令
    if (strncmp(cmd, "stor ", 5) == 0) {
        const char* arg = cmd + 5; // 指向具体的动作指令部分
        
        // 2. 纯字符串匹配，不牵涉任何变量提取
        if (strcmp(arg, "save") == 0) {
            Storage_Save();
            Debug_Log("Storage: OK, Saved.\r\n");
            return 1; 
        }
        else if (strcmp(arg, "read") == 0) {
            // 这里可以直接调用你写好的整体打印函数
            Debug_Log("Storage: OK, Read triggered.\r\n");
            return 1;
        }
        else if (strcmp(arg, "reset") == 0) {
            // 预留位置，以后可以加清空 Flash 或恢复默认值的操作
            Debug_Log("Storage: OK, Reset triggered.\r\n");
            return 1;
        }
        else {
            // 命令前缀对上了，但是后面的动作没匹配上
            Debug_Log("Storage Err: try 'save', 'read', 'reset'\r\n");
            return 1; 
        }
    }
    
    // 3. 不是 storage 的命令，原样放行，让其他模块（比如 Beep）去处理
    return 0; 
}