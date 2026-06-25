//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: i2c.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __I2C_H__
#define __I2C_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
#include "config.h"
//<<AICUBE_USER_DEFINE_END>>


void I2C1_Init(void);
void I2C1_MasterWait(void);
void I2C1_MasterStart(void);
void I2C1_MasterStop(void);
uint8_t I2C1_MasterSendByte(uint8_t dat);
uint8_t I2C1_MasterReadByte(uint8_t ack);

extern BOOL fI2C1MasterBusy;
extern uint8_t xdata pu8I2C1DMATxBuffer[16];
extern uint8_t xdata pu8I2C1DMARxBuffer[16];


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
