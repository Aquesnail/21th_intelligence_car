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


void I2C2_Init(void);
void I2C2_MasterWait(void);
void I2C2_MasterStart(void);
void I2C2_MasterStop(void);
BOOL I2C2_MasterSendByte(uint8_t dat);
uint8_t I2C2_MasterReadByte(BOOL ack);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
