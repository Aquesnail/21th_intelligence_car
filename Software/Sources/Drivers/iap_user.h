//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: iap.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __IAP_H__
#define __IAP_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
#include "config.h"
//<<AICUBE_USER_DEFINE_END>>


void IAP_Init(void);
void IAP_EraseSector(uint32_t dwAddress);
void IAP_ProgramByte(uint32_t dwAddress, uint8_t bData);
uint8_t IAP_ReadByte(uint32_t dwAddress);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
void IAP_Write_16Bytes_Aligned(uint32_t dwAddress,uint8_t far * dat);
void IAP_Write_16Bytes(uint32_t dwAddress,uint8_t far * dat);

void IAP_Read_16Bytes_Aligned(uint32_t dwAddress,uint8_t far * dat);
void IAP_Read_16Bytes(uint32_t dwAddress,uint8_t far * dat);

void IAP_Write_4Bytes(uint32_t dwAddress,uint8_t far * dat);
void IAP_Write_4Bytes_Aligned(uint32_t dwAddress,uint8_t far * dat);

void IAP_Read_4Bytes(uint32_t dwAddress,uint8_t far * dat);
void IAP_Read_4Bytes_Aligned(uint32_t dwAddress,uint8_t far * dat);

void IAP_Write_2Bytes(uint32_t dwAddress,uint8_t far * dat);
void IAP_Write_2Bytes_Aligned(uint32_t dwAddress,uint8_t far * dat);

void IAP_Read_2Bytes(uint32_t dwAddress,uint8_t far * dat);
void IAP_Read_2Bytes_Aligned(uint32_t dwAddress,uint8_t far * dat);

//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
