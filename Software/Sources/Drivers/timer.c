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
void TIMER9_Init(void)
{
#define T9_PSCR                 (9)
#define T9_RELOAD               (65536) //定时周期1微秒

    TIMER9_TimerMode();                 //设置定时器9为定时模式
    TIMER9_12TMode();                   //设置定时器9为12T模式
   // TIMER9_EnableCLKO();                //使能定时器9时钟输出功能
    TIMER9_EnableInt();
    TIMER9_SetIntPriority(1);
    TIMER9_SetPrescale(T9_PSCR);        //设置定时器9的8位预分频
    TIMER9_SetReload16(T9_RELOAD);      //设置定时器9的16位重载值
    TIMER9_Run();                       //定时器9开始运行

    //<<AICUBE_USER_TIMER9_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER9_INITIAL_END>>
}

void TIMER10_Init(void)
{
#define T10_PSCR                (200)
#define T10_RELOAD              (65536 - 50000) //定时周期1000微秒

    TIMER10_TimerMode();                //设置定时器10为定时模式
    TIMER10_12TMode();                  //设置定时器10为12T模式
    TIMER10_SetIntPriority(0);          //设置中断为最低优先级
    TIMER10_EnableInt();                //使能定时器10中断
    //TIMER10_EnableCLKO();
    TIMER10_SetPrescale(T10_PSCR);      //设置定时器10的8位预分频
    TIMER10_SetReload16(T10_RELOAD);    //设置定时器10的16位重载值
    TIMER10_Run();                      //定时器10开始运行

    //<<AICUBE_USER_TIMER10_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER10_INITIAL_END>>
}

uint16_t tim9_high_word=0;
uint32_t Get_Tim9(){
    uint16_t low;
    uint32_t high;
    
    // 防止读取时刚好发生溢出的原子操作
    do {
        high = tim9_high_word;
        low = ((uint16_t)T9H << 8)+T9L;
    } while (high != tim9_high_word);
    
    return (high << 16) | low;
    
}

void TIMER9_ISR(void) interrupt TMR9_TMR10_VECTOR
{
    if(TIMER9_CheckFlag()){
        tim9_high_word++;
        TIMER9_ClearFlag();
    }

}

////////////////////////////////////////
// 定时器11中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER11_ISR(void) interrupt TMR11_VECTOR
{
    //<<AICUBE_USER_TIMER11_ISR_CODE1_BEGIN>>
    TIMER11_ClearFlag();
    g_u16OverflowCount++;
    //<<AICUBE_USER_TIMER11_ISR_CODE1_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


