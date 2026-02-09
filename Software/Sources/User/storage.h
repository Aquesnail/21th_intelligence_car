#ifndef _STORAGE_H
#define _STORAGE_H
//system include begin//
#include "config.h"
#include "iap.h"
//system include end//

#include "vartype.h"

typedef enum {
    OUT_OF_RAM = 1,
    OUT_OF_FLASH = 2,
    NOT_REGISTERED = 3,

}StorageErr_t

void Storage_Register(void* addr);
void Storage_Load();
void Storage_Change(void* addr);
void Storage_Save();


#endif