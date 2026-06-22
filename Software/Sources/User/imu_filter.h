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
    float gx_body, gy_body, gz_body;  /* 四元数推算的重力方向 (车体坐标系, 单位向量) */
    IMU_SensorData_u sensor;  /* 共享union：包含 ax,ay,az,gx,gy,gz,temp,sample_time */
    float Kp,Ki;
    float bias_gx,bias_gy,bias_gz;
    volatile uint8_t is_calibrated;
}IMU_Filter_t;

void IMU_Filter_Update(IMU_Filter_t* dat);
void IMU_GetGravityVector(float *gx, float *gy, float *gz);




#endif