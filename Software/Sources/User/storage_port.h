#ifndef __STORAGE_PORT_H__
#define __STORAGE_PORT_H__

#include "config.h"
#include "imu_filter.h"

/* 物理扇区大小 */
#define PHY_SECTOR_SIZE    512
/* 固件预期的主版本号，若 Flash 内版本与此不符则判定为非法数据 */
#define FIRMWARE_EXPECTED_MAJOR  1 


//!!!!!这里的参数顺序，要么选择大的版本号，要么就敲定了不要动，否则会导致读取上来的数字是混乱的（子参数结构体同理)
//!!!!!如果这个结构体的地址改变，可能会导致修改变量的指令失效，一定一定要重新握手刷新内存地址到上位机后才能修改变量!!!!!!!!!

typedef struct {
    IMU_Filter_t imu_filter;
} UserParams_t;

void Params_Link(void);
void Params_Init_Default(void);
void Params_SetVersion(uint8_t major, uint8_t minor);

#endif