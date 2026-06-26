//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 文件名称: iic_user.h
// I2C1 主机模式驱动 (P7.6=SCL, P7.7=SDA)
// 支持轮询模式和 DMA 模式
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __I2C_H__
#define __I2C_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
#include "config.h"

/* DMA 缓冲区大小 */
#define I2C1_DMA_BUF_SIZE   256
//<<AICUBE_USER_DEFINE_END>>


/* ==================== 初始化 ==================== */
void I2C1_Init(void);

/* ==================== 轮询模式基本操作 ==================== */
void    I2C1_Wait(void);           /* 等待 MSIF=1 并清除 */
void    I2C1_WaitBusy(void);       /* 等待 BUSY=0 (事务开始前调用) */
void    I2C1_StartCmd(void);       /* 发送 START (独立) */
void    I2C1_StopCmd(void);        /* 发送 STOP (独立) */

/* ==================== 设备寄存器读写 (16-bit 寄存器地址) ==================== */
uint8_t  I2C1_ReadRegByte(uint8_t dev_wr_addr, uint16_t reg_addr);
uint16_t I2C1_ReadRegWord(uint8_t dev_wr_addr, uint16_t reg_addr);
void     I2C1_WriteRegByte(uint8_t dev_wr_addr, uint16_t reg_addr, uint8_t dat);

/* ==================== 轮询模式多字节读写 (VL53L1X 等) ==================== */
void I2C1_WriteRegMulti(uint8_t dev_wr_addr, uint16_t reg_addr,
                        uint8_t *pdat, uint8_t count);
void I2C1_ReadRegMulti(uint8_t dev_wr_addr, uint16_t reg_addr,
                       uint8_t *pdat, uint8_t count);

/* ==================== DMA 模式 (参照官方例程) ==================== */
void I2C1_DMA_Config(void);

/* DMA 写入: dev_wr_addr=设备写地址, mem_addr=1字节存储地址(AT24C02等), len=数据长度 */
/* 调用前需将数据填入 I2C1_DmaTxBuffer[2..2+len-1] */
void I2C1_DMA_WriteNbyte(uint8_t dev_wr_addr, uint8_t mem_addr, uint8_t len);

/* DMA 读出: dev_wr_addr=设备写地址, dev_rd_addr=设备读地址, mem_addr=1字节存储地址, len=读出长度 */
/* 数据读出到 I2C1_DmaRxBuffer[0..len-1] */
void I2C1_DMA_ReadNbyte(uint8_t dev_wr_addr, uint8_t dev_rd_addr,
                        uint8_t mem_addr, uint8_t len);

/* DMA 寄存器写入 (16-bit 寄存器地址, VL6180X等):
   调用前将数据填入 I2C1_DmaTxBuffer[3..3+len-1] */
void I2C1_DMA_WriteReg(uint8_t dev_wr_addr, uint16_t reg_addr, uint8_t len);

/* DMA 寄存器读出 (16-bit 寄存器地址): 数据读到 I2C1_DmaRxBuffer[0..len-1] */
void I2C1_DMA_ReadReg(uint8_t dev_wr_addr, uint8_t dev_rd_addr,
                      uint16_t reg_addr, uint8_t len);

/* DMA 缓冲区 & 标志 */
extern uint8_t xdata I2C1_DmaTxBuffer[I2C1_DMA_BUF_SIZE];
extern uint8_t xdata I2C1_DmaRxBuffer[I2C1_DMA_BUF_SIZE];
extern bit I2C1_DmaTxFlag;
extern bit I2C1_DmaRxFlag;


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
