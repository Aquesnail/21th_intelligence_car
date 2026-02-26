#ifndef _APP_STORAGE_H
#define _APP_STORAGE_H

#include "config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
extern SemaphoreHandle_t xStorageReadySignal;

void Storage_App_Init(void);
uint8_t Storage_HandleCommand(const char* cmd); // 返回1表示已处理，0表示未处理
#endif