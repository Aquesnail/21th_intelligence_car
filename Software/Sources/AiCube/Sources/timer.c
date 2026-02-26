//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: timer.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-02-14) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
volatile uint16_t g_u16OverflowCount = 0;  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



////////////////////////////////////////
// 定时器11初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER11_Init(void)
{
#define T11_PSCR                (99)
#define T11_RELOAD              (65536 - 65536)

    TIMER11_CLK_SYSCLK();               //定时器11使用系统时钟作为时钟源

    TIMER11_TimerMode();                //设置定时器11为定时模式
    TIMER11_12TMode();                  //设置定时器11为12T模式
    TIMER11_EnableInt();                //使能定时器11中断
    TIMER11_SetPrescale(T11_PSCR);      //设置定时器11的8位预分频
    TIMER11_SetReload16(T11_RELOAD);    //设置定时器11的16位重载值
    TIMER11_Run();                      //定时器11开始运行
    //<<AICUBE_USER_TIMER11_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER11_INITIAL_END>>
}


////////////////////////////////////////
// 定时器11中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER11_ISR(void) interrupt TMR11_VECTOR
{
    //<<AICUBE_USER_TIMER11_ISR_CODE1_BEGIN>>
    g_u16OverflowCount++; 
    //<<AICUBE_USER_TIMER11_ISR_CODE1_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


