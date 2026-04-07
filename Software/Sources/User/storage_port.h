#ifndef __STORAGE_PORT_H__
#define __STORAGE_PORT_H__

#include "config.h"
/* 物理扇区大小 */
#define PHY_SECTOR_SIZE    512

#include "imu_filter.h"
typedef struct{
    IMU_Filter_t imu_filter;

}UserParams_t;
extern UserParams_t *UserParams;
void Params_Link();
void Params_Init_Default();
#endif