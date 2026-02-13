#ifndef _STORAGE_H
#define _STORAGE_H
//system include begin//
#include "config.h"
#include "iap_user.h"
//system include end//
#include "vartype.h"

typedef enum {
    OUT_OF_RAM = 1,
    OUT_OF_FLASH = 2,
    NOT_REGISTERED = 3,

}StorageErr_t;

//void Storage_Register(void* addr);
StorageErr_t Storage_Load(void far* addr);
StorageErr_t Storage_Mark_Data(void far* addr);
void Storage_Save_Data();
StorageErr_t Storage_Register(void far * addr,VarType_t type);
void Storage_Init();


#endif