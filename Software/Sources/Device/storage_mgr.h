#ifndef __STORAGE_MGR_H__
#define __STORAGE_MGR_H__

#include "config.h"
#include "storage_port.h"

/* --- 内部包装协议 --- */
typedef struct {
    uint32_t magic;
    uint8_t  major_ver;    /* 大版本号 (0-99) */
    uint8_t  minor_ver;    /* 小版本号 (0-99) */
    uint16_t reserved;     /* 字节对齐预留 */
    UserParams_t payload;   /* 业务数据 */
    uint16_t crc;           /* 校验码 */
} Package_t;

extern Package_t g_ram_pkg;

void Storage_Init(void);
void Storage_Save(void);
void *Storage_GetPtr(void);

#endif