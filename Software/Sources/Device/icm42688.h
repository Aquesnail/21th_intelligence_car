#ifndef __ICM42688_H
#define __ICM42688_H

#include "Config.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
//#include "port.h"
#include "icm42688_defs.h"

extern uint8_t imu_ready;

/* 片选操作直接留在头文件供底层调用 */
#define ICM42688_CS_SET() P8SETB=PIN_0
#define ICM42688_CS_RESET() P8CLRB=PIN_0

#define ICM42688_SPI_TIMEOUt 1000

/* 错误码定义 */
#define ICM_OK      0
#define ICM_ERROR   1
#define ICM_TIMEOUT 2

/* --- 核心：极简的函数指针类型定义 --- */
typedef uint8_t (*spi_tx_rx_byte_t)(uint8_t dat);                 // SPI 单字节全双工收发
typedef void    (*spi_dma_read_t)(uint8_t *rx_buf, uint16_t len); // SPI DMA 批量读取
typedef void    (*icm_delay_ms_t)(uint32_t ms);                   // 毫秒延时
typedef void    (*icm_print_t)(const char* fmt,...);

/* 核心对象结构体 */
typedef struct {
    spi_tx_rx_byte_t     spi_tx_rx;      // 传入你的 SPI2_WriteReadByte 封装
    spi_dma_read_t       spi_dma_read;   // 传入你的 DMA 读函数封装
    icm_delay_ms_t       delay;
    icm_print_t          print;

    float                acc_sensitivity;
    float                gyro_sensitivity;
    uint8_t              current_bank;
    uint16_t             FIFO_THS;
    uint16_t             FIFO_Data_Process;
} ICM42688_t;

typedef struct {
   float ax,ay,az,gx,gy,gz;
   float temp;
   float timestamp;
} ICM42688_Pack16_t;

/* 初始化与配置 */
uint8_t ICM42688_Init(ICM42688_t *dev);
uint8_t ICM42688_ReadData(ICM42688_t *dev, float *acc, float *gyro, float *temp);
uint16_t ICM42688_ReadFIFO(ICM42688_t *dev, uint8_t *buffer);

void ICM42688_FIFO_Data_Handle(ICM42688_t *dev,ICM42688_Pack16_t *dat,uint8_t* ptr);
#endif