#include "storage_mgr.h"
#include "storage_port.h"
#include <string.h>
#include "iap_user.h"
#include "debug_arch.h"
/* --- 配置参数 --- */
#define STORAGE_BASE_ADDR   0x0800L
#define STORAGE_DATA_SIZE   2048
#define STORAGE_ZONE_SIZE   2048
#define ZONE_A_ADDR         STORAGE_BASE_ADDR
#define ZONE_B_ADDR         (STORAGE_BASE_ADDR + STORAGE_ZONE_SIZE)

#define MAGIC_NUM           0x5AA55AA5L

void Port_Flash_Init(void) {
    IAP_Init(); /* 调用你 iap_user.c 中的自检初始化 */
}

void Port_Flash_Erase(uint32_t addr, uint16_t sector_count) {
    uint16_t i;
    for (i = 0; i < sector_count; i++) {
        IAP_EraseSector(addr + (uint32_t)i * PHY_SECTOR_SIZE);
    }
}

void Port_Flash_Read(uint32_t addr, uint8_t far *buf, uint16_t len) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        buf[i] = IAP_ReadByte(addr + i);
    }
}

void Port_Flash_Write(uint32_t addr, uint8_t far *buf, uint16_t len) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        IAP_ProgramByte(addr + i, buf[i]);
    }
}




/* --- 静态内存缓冲 (分配在 XDATA) --- */
Package_t g_ram_pkg;

/* --- 内部工具 --- */
static uint16_t CalcCRC(uint8_t far *p, uint16_t len) {
    uint16_t res = 0;
    uint16_t i;
    for (i = 0; i < len; i++) {
        res += p[i];
    }
    return res;
}

/* 验证包的合法性 */
static uint8_t IsPackageValid(Package_t *pkg) {
    uint16_t c;
    if (pkg->magic != MAGIC_NUM) return 0;
		return 1;
}

/* --- 公开接口 --- */

void Storage_Init(void) {
    static Package_t xdata tmp_a;
    static Package_t xdata tmp_b;
    uint8_t v_a, v_b;
    Params_Link();
    Port_Flash_Init();

    /* 物理读取 */
    Port_Flash_Read(ZONE_A_ADDR, (uint8_t far *)&tmp_a, sizeof(Package_t));
    Port_Flash_Read(ZONE_B_ADDR, (uint8_t far *)&tmp_b, sizeof(Package_t));

    v_a = IsPackageValid(&tmp_a);
    v_b = IsPackageValid(&tmp_b);

    if (v_a && v_b) {
        /* 取版本号大的 */
        if (tmp_a.version >= tmp_b.version) {
            memcpy(&g_ram_pkg, &tmp_a, sizeof(Package_t));
        } else {
            memcpy(&g_ram_pkg, &tmp_b, sizeof(Package_t));
        }
    } else if (v_a) {
        memcpy(&g_ram_pkg, &tmp_a, sizeof(Package_t));
    } else if (v_b) {
        memcpy(&g_ram_pkg, &tmp_b, sizeof(Package_t));
    } else {
        /* 均失效，执行“工厂初始化” */
        //Debug_Log("Storage: Flash Invalid, Set Default.\r\n");
        memset(&g_ram_pkg, 0, sizeof(Package_t)); // 清零基础结构
        g_ram_pkg.magic = MAGIC_NUM;
        Params_Init_Default(); // 填入你的 1.2, 0.002 等变量
    }
}

void Storage_Save(void) {
    uint32_t target_addr;
    static Package_t xdata old_pkg;
    uint8_t v_a;

    /* 先递增版本号 - 确保每次 save 都产生新版本 */
    g_ram_pkg.version++;
    g_ram_pkg.magic = MAGIC_NUM;
    g_ram_pkg.crc = CalcCRC((uint8_t far *)&g_ram_pkg, sizeof(Package_t) - 2);

    /* 决定写入哪个区 - Ping-Pong 策略 */
    Port_Flash_Read(ZONE_A_ADDR, (uint8_t far *)&old_pkg, sizeof(Package_t));
    v_a = IsPackageValid(&old_pkg);

    /* 如果 A 区有效且版本号等于当前内存版本（说明上次写的是A），这次写B */
    if (v_a && (old_pkg.version == g_ram_pkg.version - 1)) {
        target_addr = ZONE_B_ADDR;
    } else {
        target_addr = ZONE_A_ADDR;
    }

    /* 物理操作：擦除 + 写入 */
    Port_Flash_Erase(target_addr, STORAGE_ZONE_SIZE / PHY_SECTOR_SIZE);
    Port_Flash_Write(target_addr, (uint8_t far *)&g_ram_pkg, sizeof(Package_t));
}

void *Storage_GetPtr(void) {
    return (void *)(&(g_ram_pkg.payload));
}