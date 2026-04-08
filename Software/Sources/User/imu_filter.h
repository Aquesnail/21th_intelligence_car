#ifndef _IMU_FILTER_H
#define _IMU_FILTER_H

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"


typedef struct{
    float pitch;
    float yaw;
    float roll;
    float gx,gy,gz,ax,ay,az;
    float temp;
    float sample_time;
    float Kp,Ki;
    float bias_gx,bias_gy,bias_gz;
    uint8_t is_calibrated;
}IMU_Filter_t;

void IMU_Filter_Update(IMU_Filter_t* dat);




#endif