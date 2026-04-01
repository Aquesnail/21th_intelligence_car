//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: uart.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-01-18) 创建文件
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
// 串口4初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART4_Init(void)
{
#ifdef BAUDRATE
#undef BAUDRATE
#endif
#define BAUDRATE                (115200UL)
#define T2_RELOAD               (65536 - (SYSCLK / BAUDRATE + 2) / 4)

    UART4_SwitchP5253();                //设置串口数据端口: RxD4 (P5.2), TxD4 (P5.3)

    UART4_Timer2BRT();                  //选择定时器2作为串口4波特率发生器
    TIMER2_TimerMode();                 //设置定时器2为定时模式
    TIMER2_1TMode();                    //设置定时器2为1T模式
    TIMER2_SetPrescale(0);              //设置定时器2的8位预分频
    TIMER2_SetReload16(T2_RELOAD);      //设置定时器2的16位重载值
    TIMER2_Run();                       //定时器2开始运行

    UART4_EnableRx();                   //使能串口4接收数据
    UART4_Mode1();                      //设置串口4为模式1 (8位数据可变波特率)

    //<<AICUBE_USER_UART4_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_UART4_INITIAL_END>>
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
uint8_t UART5_Write(char* str,uint16_t len){
	return 0;

}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


