#include "adc_user.h"

extern void IMU_Get_Angle(float* roll,float* pitch,float* yaw);

//以ADC的DMA回调为时间点开始运算
uint8_t xdata adc2_dma_buffer[ADC2_CH_NUM][ADC2_BUF_SIZE];

void vMainControlTask(void* params){
    (void)params;
}