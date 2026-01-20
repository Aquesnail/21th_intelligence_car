//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: crc.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-01-19) 创建文件
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
// CRC初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void CRC_Init(void)
{
    CRC_Reset();                        //复位CRC模块
    CRC_ModebusMode();                  //设置CRC为16位MODEBUS规范
    CRC_DataReverse();                  //输入数据需要反转（高低位交换）
    CRC_ResultReverse();                //计算结果需要反转（高低位交换）
    CRC_ResultNonInverted();            //计算结果不需要取反
    CRC_SetInitial16(0xFFFF);           //设置CRC计算初始值

    //<<AICUBE_USER_CRC_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_CRC_INITIAL_END>>
}

////////////////////////////////////////
// CRC初始化函数
// 入口参数: dat  (待CRC计算的数据)
//           size (数据大小)
// 函数返回: CRC计算结果 (若为MODEBUS模式，则结果低16位有效)
////////////////////////////////////////
uint32_t CRC_Calculate(uint8_t *dat, int size)
{
    while (size--)                      //判断数据是否解释
        CRC_DataIn(*dat++);             //计算下一个数据

    return (CRC_IsCRC32() ? CRC_ReadResult32() :
                            CRC_ReadResult16()); //根据模式返回结果
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


