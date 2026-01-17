//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: lvr.c
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
// 低压复位初初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void LVR_Init(void)
{
    RST_ResetPinAsGPIO();               //RST管脚用作普通I/O口 (P5.4)

    LVR_SetLVDLevelV2p7();              //设置低压检测门槛电压位2.7V
    LVR_EnableLVR();                    //使能LVR低压复位


    //<<AICUBE_USER_LVR_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_LVR_INITIAL_END>>
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


