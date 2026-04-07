#ifndef __STORAGE_MGR_H__
#define __STORAGE_MGR_H__

#include "config.h"
#include "storage_port.h"
/* --- 内部包装协议 --- */
typedef struct {
    uint32_t magic;
    uint32_t version;       /* 写入版本号/计数器 */
    UserParams_t payload;   /* 业务数据 */
    uint16_t crc;           /* 校验码 */
} Package_t;
extern Package_t g_ram_pkg;
/* 初始化并加载数据 */
void Storage_Init(void);

/* 保存当前内存数据到 Flash */
void Storage_Save(void);

/* 获取业务数据结构体指针 */
void *Storage_GetPtr(void);

#endif