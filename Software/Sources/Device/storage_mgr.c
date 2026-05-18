#include "storage_mgr.h"
#include "storage_port.h"
#include <string.h>
#include "iap_user.h"
#include "debug_arch.h"

#define STORAGE_BASE_ADDR   64*1024
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
    Params_Init_Default();
}

void Storage_Save(void) {
    (void)0;
}

void *Storage_GetPtr(void) {
    return (void *)(&(g_ram_pkg.payload));
}