#ifndef _STORAGE_H
#define _STORAGE_H
//system include begin//
#include "config.h"
#include "iap_user.h"
//system include end//
#include <stdint.h>  // 添加以使用uint16_t等

typedef enum {
    OUT_OF_RAM = 1,
    OUT_OF_FLASH = 2,
    NOT_REGISTERED = 3,
    NOT_IN_FLASH   = 4,
} StorageErr_t;

typedef void    (*storage_print_t)(const char* fmt,...);
typedef void    (*storage_asyn_delay_t)(uint16_t t);
typedef void    (*storage_delay_t)(uint16_t t);
typedef uint8_t (*storage_read_bytes)(uint32_t flash_addr,void* ram_addr,uint16_t len);
typedef uint8_t (*storage_write_bytes)(uint32_t flash_addr,void* ram_addr,uint16_t len);

// typedef struct{
//     // 可能用于初始化配置，目前为空
// } Storage_Init_t;

StorageErr_t Storage_Data_Writed(void far* addr);
StorageErr_t Storage_Load(void far* addr);
StorageErr_t Storage_Mark_Data(void far* addr);
void Storage_Save_Data(void);
StorageErr_t Storage_Register(void far * addr, uint16_t len, char* name);
void Storage_Init(void);

#endif