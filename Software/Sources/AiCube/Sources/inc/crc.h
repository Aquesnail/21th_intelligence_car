//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: crc.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __CRC_H__
#define __CRC_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
#include "config.h"
//<<AICUBE_USER_DEFINE_END>>


void CRC_Init(void);
uint32_t CRC_Calculate(uint8_t *dat, int len);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
