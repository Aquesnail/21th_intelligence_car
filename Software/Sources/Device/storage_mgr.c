#include "storage_mgr.h"
#include "storage_port.h"
#include <string.h>
#include "iap_user.h"
#include "debug_arch.h"

#define STORAGE_BASE_ADDR   0x0800L
#define STORAGE_ZONE_SIZE   2048
#define ZONE_A_ADDR         STORAGE_BASE_ADDR
#define ZONE_B_ADDR         (STORAGE_BASE_ADDR + STORAGE_ZONE_SIZE)
#define MAGIC_NUM           0x5AA55AA5L

Package_t g_ram_pkg = {0};

void Port_Flash_Init(void) {
   
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

static uint16_t CalcCRC(uint8_t far *p, uint16_t len) {
    uint16_t res = 0;
    uint16_t i;
    for (i = 0; i < len; i++) {
        res += p[i];
    }
    return res;
}

static uint8_t IsPackageValid(Package_t *pkg) {
    uint16_t cal_crc;
    if (pkg->magic != MAGIC_NUM) return 0;
    cal_crc = CalcCRC((uint8_t *)pkg, sizeof(Package_t) - 2);
    if (cal_crc == pkg->crc) {
        return 1; // 校验成功
    } else {
        return 0; // 校验失败
    }
    return 1;
}

void Storage_Init(void) {
    /* C89: 变量声明在最前 */
    static Package_t tmp_a;
    static Package_t tmp_b;
    uint8_t v_a, v_b;

    Params_Link();
    IAP_Init();

    Port_Flash_Read(ZONE_A_ADDR, (uint8_t far *)&tmp_a, sizeof(Package_t));
    Port_Flash_Read(ZONE_B_ADDR, (uint8_t far *)&tmp_b, sizeof(Package_t));

    v_a = IsPackageValid(&tmp_a);
    v_b = IsPackageValid(&tmp_b);

    /* 核心修改：大版本不一致则视为无效，不移入内存 */
    if (v_a && (tmp_a.major_ver != FIRMWARE_EXPECTED_MAJOR)) v_a = 0;
    if (v_b && (tmp_b.major_ver != FIRMWARE_EXPECTED_MAJOR)) v_b = 0;
    memset(&g_ram_pkg, 0, sizeof(Package_t));
    g_ram_pkg.magic = MAGIC_NUM;
    Params_Init_Default();
    if (v_a && v_b) {
        /* 取小版本号大的 */
        if (tmp_a.minor_ver >= tmp_b.minor_ver) {
            memcpy(&g_ram_pkg, &tmp_a, sizeof(Package_t));
        } else {
            memcpy(&g_ram_pkg, &tmp_b, sizeof(Package_t));
        }
    } else if (v_a) {
        memcpy(&g_ram_pkg, &tmp_a, sizeof(Package_t));
    } else if (v_b) {
        memcpy(&g_ram_pkg, &tmp_b, sizeof(Package_t));
    } else {
        return;
    }
}

void Storage_Save(void) {
    uint32_t target_addr;
    static Package_t xdata old_pkg;
    uint8_t v_a;

    /* 确保写入的是最新的版本标记 */
    g_ram_pkg.magic = MAGIC_NUM;
    /* CRC 排除末尾的 2 字节校验位自身 */
    g_ram_pkg.crc = CalcCRC((uint8_t far *)&g_ram_pkg, sizeof(Package_t) - 2);

    Port_Flash_Read(ZONE_A_ADDR, (uint8_t far *)&old_pkg, sizeof(Package_t));
    v_a = IsPackageValid(&old_pkg);

    /* Ping-Pong 逻辑 */
    if (v_a && (old_pkg.minor_ver == g_ram_pkg.minor_ver)) {
        /* 如果 A 区版本已经和当前一致，说明上次写的是 A，这次换 B */
        target_addr = ZONE_B_ADDR;
    } else {
        target_addr = ZONE_A_ADDR;
    }

    Port_Flash_Erase(target_addr, STORAGE_ZONE_SIZE / PHY_SECTOR_SIZE);
    Port_Flash_Write(target_addr, (uint8_t far *)&g_ram_pkg, sizeof(Package_t));
}

void *Storage_GetPtr(void) {
    return (void *)(&(g_ram_pkg.payload));
}