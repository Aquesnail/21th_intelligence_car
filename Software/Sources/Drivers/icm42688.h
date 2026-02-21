#ifndef __ICM42688_H
#define __ICM42688_H

#include "Config.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "port.h"
#include "icm42688_defs.h"


//#define ICM;
extern uint8_t imu_ready;
#define ICM42688_CS_SET() P45 = 1
#define ICM42688_CS_RESET() P45 = 0

#define ICM42688_SPI_TIMEOUt 1000
/* 错误码定义 */
#define ICM_OK      0
#define ICM_ERROR   1
#define ICM_TIMEOUT 2

/* 函数指针类型定义 */
typedef uint8_t (*icm_write_reg_t)(uint8_t reg, uint8_t dat);
typedef uint8_t (*icm_read_reg_t)(uint8_t reg, uint8_t * dat, uint16_t len);
typedef uint8_t (*icm_read_reg_byte_t)(uint8_t reg, uint8_t *dat);  // 新增
typedef void    (*icm_delay_ms_t)(uint32_t ms);

/* 核心对象结构体 */
typedef struct {
    icm_write_reg_t      write;
    icm_read_reg_t       read;
    icm_read_reg_byte_t  read_byte;    // 新增
    icm_delay_ms_t       delay;
    float                acc_sensitivity;
    float                gyro_sensitivity;
    uint8_t              current_bank;
    uint16_t             FIFO_THS;
    uint16_t             FIFO_Data_Process;
} ICM42688_t;

/* 初始化与配置 */
uint8_t ICM42688_Init(ICM42688_t *dev);
uint8_t ICM42688_ReadData(ICM42688_t *dev, float *acc, float *gyro, float *temp);
uint16_t ICM42688_ReadFIFO(ICM42688_t *dev, uint8_t *buffer);

#endif


