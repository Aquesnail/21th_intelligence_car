#ifndef __ICM42688_H
#define __ICM42688_H

#include "Config.h"
#include "math.h"
#include "icm42688_defs.h"

extern uint8_t imu_ready;

/* 片选操作 */
#define ICM42688_CS_SET()   P8SETB = PIN_0
#define ICM42688_CS_RESET() P8CLRB = PIN_0

#define ICM42688_SPI_TIMEOUT  1000

/* 错误码 */
#define ICM_OK      0
#define ICM_ERROR   1
#define ICM_TIMEOUT 2

/* =========================================================
 * 外部函数声明：在 imu_port.c 中定义
 * =========================================================*/
extern uint8_t ICM42688_SPI_TxRx(uint8_t dat);           /* SPI单字节收发 */
extern void    ICM42688_SPI_DmaRead(uint8_t *rx_buf, uint16_t len);  /* SPI DMA读取 */
extern void    ICM42688_DelayMs(uint32_t ms);            /* 毫秒延时 */
extern uint32_t ICM42688_GetTick(void);                   /* 获取系统 tick */
extern void    ICM42688_Print(const char* fmt, ...);     /* 调试打印 */

/* =========================================================
 * 外部变量声明：在 imu_port.c 中定义
 * =========================================================*/
extern float   icm_acc_sensitivity;    /* 加速度灵敏度 */
extern float   icm_gyro_sensitivity;   /* 陀螺仪灵敏度 */
extern uint8_t icm_current_bank;       /* 当前寄存器bank */
extern uint16_t icm_fifo_ths;          /* FIFO阈值 */

/* =========================================================
 * Union共享传感器数据：IMU和滤波器直接共享，零拷贝
 * 内存布局: ax,ay,az,gx,gy,gz,temp,sample_time (8个float)
 * =========================================================*/
typedef union {
    struct {
        float ax, ay, az;
        float gx, gy, gz;
        float temp;
        float sample_time;
    } dat;
    float raw[8];
} IMU_SensorData_u;

typedef struct {
   IMU_SensorData_u sensor;   /* 共享union */
   float timestamp;           /* 额外时间戳 */
} ICM42688_Pack16_t;

/* 初始化与数据处理 */
uint8_t  ICM42688_Init(void);
uint8_t  ICM42688_ReadData(float *acc, float *gyro, float *temp);
uint16_t ICM42688_ReadFIFO(uint8_t *buffer);
uint16_t ICM42688_ReadFIFO_Blocking(uint8_t *buffer, uint16_t pack_count, uint32_t timeout_ms);
void     ICM42688_FIFO_Data_Handle(ICM42688_Pack16_t *dat, uint8_t *ptr);

#endif
