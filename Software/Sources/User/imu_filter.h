#ifndef _IMU_FILTER_H
#define _IMU_FILTER_H

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

#include "icm42688.h"  /* 引入共享的union定义 */

typedef struct{
    float pitch;
    float yaw;
    float roll;
    IMU_SensorData_u sensor;  /* 共享union：包含 ax,ay,az,gx,gy,gz,temp,sample_time */
    float Kp,Ki;
    float bias_gx,bias_gy,bias_gz;
    uint8_t is_calibrated;
}IMU_Filter_t;

void IMU_Filter_Update(IMU_Filter_t* dat);




#endif