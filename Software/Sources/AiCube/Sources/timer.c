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
// 定时器0初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER0_Init(void)
{
#define T0_PSCR                 (0)
#define T0_RELOAD               (65536 - (float)SYSCLK / 12 / (T0_PSCR + 1) * 100 / 1000000) //定时周期100微秒

    TIMER0_TimerMode();                 //设置定时器0为定时模式
    TIMER0_12TMode();                   //设置定时器0为12T模式
    TIMER0_Mode0();                     //设置定时器0为模式0 (16位自动重载模式)
    TIMER0_DisableGateINT0();           //禁止定时器0门控
    TIMER0_SetPrescale(T0_PSCR);        //设置定时器0的8位预分频
    TIMER0_SetReload16(T0_RELOAD);      //设置定时器0的16位重载值
    TIMER0_Run();                       //定时器0开始运行

    //<<AICUBE_USER_TIMER0_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER0_INITIAL_END>>
}

////////////////////////////////////////
// 定时器8初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER8_Init(void)
{
#define T8_PSCR                 (0)
#define T8_RELOAD               (65536 - (float)SYSCLK / 12 / (T8_PSCR + 1) * 1000 / 1000000) //定时周期1000微秒

    TIMER8_TimerMode();                 //设置定时器8为定时模式
    TIMER8_12TMode();                   //设置定时器8为12T模式
    TIMER8_SetIntPriority(0);           //设置中断为最低优先级
    TIMER8_EnableInt();                 //使能定时器8中断
    TIMER8_SetPrescale(T8_PSCR);        //设置定时器8的8位预分频
    TIMER8_SetReload16(T8_RELOAD);      //设置定时器8的16位重载值
    TIMER8_Run();                       //定时器8开始运行

    //<<AICUBE_USER_TIMER8_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER8_INITIAL_END>>
}

////////////////////////////////////////
// 定时器10初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER10_Init(void)
{
#define T10_PSCR                (0)
#define T10_RELOAD              (65536 - (float)SYSCLK / 12 / (T10_PSCR + 1) * 1000 / 1000000) //定时周期1000微秒

    TIMER10_TimerMode();                //设置定时器10为定时模式
    TIMER10_12TMode();                  //设置定时器10为12T模式
    TIMER10_SetIntPriority(0);          //设置中断为最低优先级
    TIMER10_EnableInt();                //使能定时器10中断
    TIMER10_SetPrescale(T10_PSCR);      //设置定时器10的8位预分频
    TIMER10_SetReload16(T10_RELOAD);    //设置定时器10的16位重载值
    TIMER10_Run();                      //定时器10开始运行

    //<<AICUBE_USER_TIMER10_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER10_INITIAL_END>>
}

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
// TIMER7_TIMER8中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER7_TIMER8_ISR(void) interrupt TMR7_TMR8_VECTOR
{
    //<<AICUBE_USER_TIMER8_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    if (TIMER8_CheckFlag())             //判断定时器8中断
    {
        TIMER8_ClearFlag();             //清除定时器8中断标志
    }
    //<<AICUBE_USER_TIMER8_ISR_CODE1_END>>
}

////////////////////////////////////////
// TIMER9_TIMER10中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER9_TIMER10_ISR(void) interrupt TMR9_TMR10_VECTOR
{
    //<<AICUBE_USER_TIMER10_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    if (TIMER10_CheckFlag())            //判断定时器10中断
    {
        TIMER10_ClearFlag();            //清除定时器10中断标志
    }
    //<<AICUBE_USER_TIMER10_ISR_CODE1_END>>
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


