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
    uint16_t time_stamp;
    float temp;
    float sample_time;
    float Kp,Ki;
}IMU_Filter_t;

void IMU_Filter_Update(IMU_Filter_t* dat);




#endif