//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: port.c
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
// P0口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT0_Init(void)
{
    SetP0nInitLevelHigh(PIN_ALL);       //设置P0初始化电平
    SetP0nQuasiMode(PIN_ALL);           //设置P0为无弱上拉准双向口模式

    DisableP0nPullUp(PIN_ALL);          //关闭P0内部上拉电阻
    DisableP0nPullDown(PIN_ALL);        //关闭P0内部下拉电阻
    DisableP0nSchmitt(PIN_ALL);         //使能P0施密特触发
    SetP0nSlewRateNormal(PIN_ALL);      //设置P0一般翻转速度
    SetP0nDrivingNormal(PIN_ALL);       //设置P0一般驱动能力
    SetP0nDigitalInput(PIN_ALL);        //使能P0数字信号输入功能

    //<<AICUBE_USER_PORT0_INITIAL_BEGIN>>
    // 1
    //<<AICUBE_USER_PORT0_INITIAL_END>>
}

////////////////////////////////////////
// P1口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT1_Init(void)
{
    SetP1nInitLevelHigh(PIN_ALL);       //设置P1初始化电平
    SetP1nQuasiMode(PIN_5 | PIN_4 | PIN_2 | PIN_1 | PIN_0); //设置P1.5,P1.4,P1.2,P1.1,P1.0为无弱上拉准双向口模式
    SetP1nHighZInputMode(PIN_7 | PIN_6 | PIN_3); //设置P1.7,P1.6,P1.3为高阻输入模式

    DisableP1nPullUp(PIN_ALL);          //关闭P1内部上拉电阻
    DisableP1nPullDown(PIN_ALL);        //关闭P1内部下拉电阻
    DisableP1nSchmitt(PIN_ALL);         //使能P1施密特触发
    SetP1nSlewRateNormal(PIN_ALL);      //设置P1一般翻转速度
    SetP1nDrivingNormal(PIN_ALL);       //设置P1一般驱动能力
    SetP1nDigitalInput(PIN_5 | PIN_4 | PIN_2 | PIN_1 | PIN_0); //使能P1.5,P1.4,P1.2,P1.1,P1.0数字信号输入功能
    SetP1nAnalogInput(PIN_7 | PIN_6 | PIN_3); //使能P1.7,P1.6,P1.3模拟信号输入功能

    //<<AICUBE_USER_PORT1_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT1_INITIAL_END>>
}

////////////////////////////////////////
// P2口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT2_Init(void)
{
    SetP2nInitLevelHigh(PIN_ALL);       //设置P2初始化电平
    SetP2nQuasiMode(PIN_ALL);           //设置P2为无弱上拉准双向口模式

    DisableP2nPullUp(PIN_ALL);          //关闭P2内部上拉电阻
    DisableP2nPullDown(PIN_ALL);        //关闭P2内部下拉电阻
    DisableP2nSchmitt(PIN_ALL);         //使能P2施密特触发
    SetP2nSlewRateNormal(PIN_ALL);      //设置P2一般翻转速度
    SetP2nDrivingNormal(PIN_ALL);       //设置P2一般驱动能力
    SetP2nDigitalInput(PIN_ALL);        //使能P2数字信号输入功能

    //<<AICUBE_USER_PORT2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT2_INITIAL_END>>
}

////////////////////////////////////////
// P3口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT3_Init(void)
{
    SetP3nInitLevelHigh(PIN_ALL);       //设置P3初始化电平
    SetP3nQuasiMode(PIN_ALL);           //设置P3为无弱上拉准双向口模式

    DisableP3nPullUp(PIN_ALL);          //关闭P3内部上拉电阻
    DisableP3nPullDown(PIN_ALL);        //关闭P3内部下拉电阻
    DisableP3nSchmitt(PIN_ALL);         //使能P3施密特触发
    SetP3nSlewRateNormal(PIN_ALL);      //设置P3一般翻转速度
    SetP3nDrivingNormal(PIN_ALL);       //设置P3一般驱动能力
    SetP3nDigitalInput(PIN_ALL);        //使能P3数字信号输入功能

    //<<AICUBE_USER_PORT3_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT3_INITIAL_END>>
}

////////////////////////////////////////
// P4口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT4_Init(void)
{
    SetP4nInitLevelHigh(PIN_ALL);       //设置P4初始化电平
    SetP4nQuasiMode(PIN_ALL);           //设置P4为无弱上拉准双向口模式

    DisableP4nPullUp(PIN_ALL);          //关闭P4内部上拉电阻
    DisableP4nPullDown(PIN_ALL);        //关闭P4内部下拉电阻
    DisableP4nSchmitt(PIN_ALL);         //使能P4施密特触发
    SetP4nSlewRateNormal(PIN_ALL);      //设置P4一般翻转速度
    SetP4nDrivingNormal(PIN_ALL);       //设置P4一般驱动能力
    SetP4nDigitalInput(PIN_ALL);        //使能P4数字信号输入功能

    //<<AICUBE_USER_PORT4_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT4_INITIAL_END>>
}

////////////////////////////////////////
// P5口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT5_Init(void)
{
    SetP5nInitLevelHigh(PIN_ALL);       //设置P5初始化电平
    SetP5nQuasiMode(PIN_7 | PIN_6 | PIN_5 | PIN_3 | PIN_2 | PIN_1 | PIN_0); //设置P5.7,P5.6,P5.5,P5.3,P5.2,P5.1,P5.0为无弱上拉准双向口模式
    SetP5nPushPullMode(PIN_4);          //设置P5.4为推挽输出模式

    DisableP5nPullUp(PIN_ALL);          //关闭P5内部上拉电阻
    DisableP5nPullDown(PIN_ALL);        //关闭P5内部下拉电阻
    DisableP5nSchmitt(PIN_ALL);         //使能P5施密特触发
    SetP5nSlewRateNormal(PIN_7 | PIN_6 | PIN_5 | PIN_3 | PIN_2 | PIN_1 | PIN_0); //设置P5.7,P5.6,P5.5,P5.3,P5.2,P5.1,P5.0一般翻转速度
    SetP5nSlewRateFast(PIN_4);          //设置P5.4快速翻转速度
    SetP5nDrivingNormal(PIN_ALL);       //设置P5一般驱动能力
    SetP5nDigitalInput(PIN_ALL);        //使能P5数字信号输入功能

    //<<AICUBE_USER_PORT5_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT5_INITIAL_END>>
}

////////////////////////////////////////
// P6口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT6_Init(void)
{
    SetP6nInitLevelHigh(PIN_ALL);       //设置P6初始化电平
    SetP6nQuasiMode(PIN_7 | PIN_5 | PIN_3 | PIN_2 | PIN_1 | PIN_0); //设置P6.7,P6.5,P6.3,P6.2,P6.1,P6.0为无弱上拉准双向口模式
    SetP6nPushPullMode(PIN_6 | PIN_4);  //设置P6.6,P6.4为推挽输出模式

    DisableP6nPullUp(PIN_ALL);          //关闭P6内部上拉电阻
    DisableP6nPullDown(PIN_ALL);        //关闭P6内部下拉电阻
    DisableP6nSchmitt(PIN_ALL);         //使能P6施密特触发
    SetP6nSlewRateNormal(PIN_7 | PIN_5 | PIN_3 | PIN_2 | PIN_1 | PIN_0); //设置P6.7,P6.5,P6.3,P6.2,P6.1,P6.0一般翻转速度
    SetP6nSlewRateFast(PIN_6 | PIN_4);  //设置P6.6,P6.4快速翻转速度
    SetP6nDrivingNormal(PIN_ALL);       //设置P6一般驱动能力
    SetP6nDigitalInput(PIN_ALL);        //使能P6数字信号输入功能

    //<<AICUBE_USER_PORT6_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT6_INITIAL_END>>
}

////////////////////////////////////////
// P7口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT7_Init(void)
{
    SetP7nInitLevelHigh(PIN_ALL);       //设置P7初始化电平
    SetP7nQuasiMode(PIN_ALL);           //设置P7为无弱上拉准双向口模式

    DisableP7nPullUp(PIN_ALL);          //关闭P7内部上拉电阻
    DisableP7nPullDown(PIN_ALL);        //关闭P7内部下拉电阻
    DisableP7nSchmitt(PIN_ALL);         //使能P7施密特触发
    SetP7nSlewRateNormal(PIN_ALL);      //设置P7一般翻转速度
    SetP7nDrivingNormal(PIN_ALL);       //设置P7一般驱动能力
    SetP7nDigitalInput(PIN_ALL);        //使能P7数字信号输入功能

    //<<AICUBE_USER_PORT7_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT7_INITIAL_END>>
}

////////////////////////////////////////
// P8口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT8_Init(void)
{
    SetP8nInitLevelHigh(PIN_ALL);       //设置P8初始化电平
    SetP8nQuasiMode(PIN_ALL);           //设置P8为无弱上拉准双向口模式

    DisableP8nPullUp(PIN_ALL);          //关闭P8内部上拉电阻
    DisableP8nPullDown(PIN_ALL);        //关闭P8内部下拉电阻
    DisableP8nSchmitt(PIN_ALL);         //使能P8施密特触发
    SetP8nSlewRateNormal(PIN_ALL);      //设置P8一般翻转速度
    SetP8nDrivingNormal(PIN_ALL);       //设置P8一般驱动能力
    SetP8nDigitalInput(PIN_ALL);        //使能P8数字信号输入功能

    //<<AICUBE_USER_PORT8_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT8_INITIAL_END>>
}

////////////////////////////////////////
// P9口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT9_Init(void)
{
    SetP9nInitLevelHigh(PIN_ALL);       //设置P9初始化电平
    SetP9nQuasiMode(PIN_7 | PIN_6 | PIN_4 | PIN_3 | PIN_2 | PIN_1 | PIN_0); //设置P9.7,P9.6,P9.4,P9.3,P9.2,P9.1,P9.0为无弱上拉准双向口模式
    SetP9nPushPullMode(PIN_5);          //设置P9.5为推挽输出模式

    DisableP9nPullUp(PIN_ALL);          //关闭P9内部上拉电阻
    DisableP9nPullDown(PIN_ALL);        //关闭P9内部下拉电阻
    DisableP9nSchmitt(PIN_ALL);         //使能P9施密特触发
    SetP9nSlewRateNormal(PIN_ALL);      //设置P9一般翻转速度
    SetP9nDrivingNormal(PIN_ALL);       //设置P9一般驱动能力
    SetP9nDigitalInput(PIN_ALL);        //使能P9数字信号输入功能

    //<<AICUBE_USER_PORT9_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT9_INITIAL_END>>
}

////////////////////////////////////////
// PA口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORTA_Init(void)
{
    SetPAnInitLevelHigh(PIN_ALL);       //设置PA初始化电平
    SetPAnQuasiMode(PIN_ALL);           //设置PA为无弱上拉准双向口模式

    DisablePAnPullUp(PIN_ALL);          //关闭PA内部上拉电阻
    DisablePAnPullDown(PIN_ALL);        //关闭PA内部下拉电阻
    DisablePAnSchmitt(PIN_ALL);         //使能PA施密特触发
    SetPAnSlewRateNormal(PIN_ALL);      //设置PA一般翻转速度
    SetPAnDrivingNormal(PIN_ALL);       //设置PA一般驱动能力
    SetPAnDigitalInput(PIN_ALL);        //使能PA数字信号输入功能

    //<<AICUBE_USER_PORT10_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT10_INITIAL_END>>
}

////////////////////////////////////////
// PB口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORTB_Init(void)
{
    SetPBnInitLevelHigh(PIN_ALL);       //设置PB初始化电平
    SetPBnQuasiMode(PIN_ALL);           //设置PB为无弱上拉准双向口模式

    DisablePBnPullUp(PIN_ALL);          //关闭PB内部上拉电阻
    DisablePBnPullDown(PIN_ALL);        //关闭PB内部下拉电阻
    DisablePBnSchmitt(PIN_ALL);         //使能PB施密特触发
    SetPBnSlewRateNormal(PIN_ALL);      //设置PB一般翻转速度
    SetPBnDrivingNormal(PIN_ALL);       //设置PB一般驱动能力
    SetPBnDigitalInput(PIN_ALL);        //使能PB数字信号输入功能

    //<<AICUBE_USER_PORT11_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT11_INITIAL_END>>
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


