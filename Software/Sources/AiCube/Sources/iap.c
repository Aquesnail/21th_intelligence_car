//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: iap.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-01-17) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



////////////////////////////////////////
// IAP初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void IAP_Init(void)
{
    IAP_Enable();                       //使能EEPROM操作
    IAP_Idle();                         //设置EEPROM为空闲模式

    //<<AICUBE_USER_IAP_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_IAP_INITIAL_END>>
}
////////////////////////////////////////
// EEPROM扇区擦除函数
// 入口参数: dwAddress (EEPROM目标扇区地址)
// 函数返回: 无
////////////////////////////////////////
void IAP_EraseSector(uint32_t dwAddress)
{
    IAP_SetAddress(dwAddress);          //设置EEPROM目标地址
    IAP_TriggerErase();                 //触发EEPROM扇区擦除
    IAP_Idle();                         //恢复EEPROM空闲模式
}

////////////////////////////////////////
// EEPROM字节编程函数
// 入口参数: dwAddress (EEPROM目标字节地址)
//           bData (待写入的字节数据)
// 函数返回: 无
////////////////////////////////////////
void IAP_ProgramByte(uint32_t dwAddress, uint8_t bData)
{
    IAP_SetAddress(dwAddress);          //设置EEPROM目标地址
    IAP_SetData(bData);                 //设置EEPROM数据
    IAP_TriggerProgram();               //触发EEPROM字节编程
    IAP_Idle();                         //恢复EEPROM空闲模式
}

////////////////////////////////////////
// EEPROM字节读取函数
// 入口参数: dwAddress (EEPROM目标字节地址)
// 函数返回: (读取的字节数据)
////////////////////////////////////////
uint8_t IAP_ReadByte(uint32_t dwAddress)
{
    uint8_t dat;
    uint32_t addr;

    addr = dwAddress + IAP_OFFSET;      //计算EEPROM地址
    dat = *(uint8_t ecode *)addr;       //MOV方式读取EEPROM数据

    return dat;                         //返回读取的数据
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
void IAP_Write_16Bytes(uint32_t dwAddress,uint8_t far * dat){
    uint8_t i;
    uint8_t volatile far *pIapData = &IAP_XDATA0;
    IAP_SetAddress(dwAddress);
    for(i=0;i<16;i++){
       pIapData[i]=dat[i];
    }
    IAP_TriggerProgram16Bytes();
    IAP_Idle();  
}
void IAP_Read_16Bytes(uint32_t dwAddress,uint8_t far * dat){
    uint8_t i;
    uint8_t volatile far *pIapData = &IAP_XDATA0;
    IAP_SetAddress(dwAddress);
    IAP_TriggerRead16Bytes();
    for(i=0;i<16;i++){
        dat[i]=pIapData[i];
    }
    IAP_Idle();
}
void IAP_Read_4Bytes(uint32_t dwAddress,uint8_t far * dat){
    uint8_t i;
    uint8_t volatile far *pIapData = &IAP_XDATA0;
    IAP_SetAddress(dwAddress);
    IAP_TriggerRead4Bytes();
    for(i=0;i<4;i++){
        dat[i]=pIapData[i];
    }
    IAP_Idle();
}
void IAP_Read_2Bytes(uint32_t dwAddress,uint8_t far * dat){
    uint8_t i;
    uint8_t volatile far *pIapData = &IAP_XDATA0;
    IAP_SetAddress(dwAddress);
    IAP_TriggerRead4Bytes();
    for(i=0;i<2;i++){
        dat[i]=pIapData[i];
    }
    IAP_Idle();
}

// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


