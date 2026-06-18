//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: port.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-06-18) 创建文件
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
    SetP0nQuasiMode(PIN_6 | PIN_4 | PIN_1 | PIN_0); //设置P0.6,P0.4,P0.1,P0.0为无弱上拉准双向口模式
    SetP0nPushPullMode(PIN_7 | PIN_5 | PIN_3 | PIN_2); //设置P0.7,P0.5,P0.3,P0.2为推挽输出模式
    SetP0nManualMode(PIN_ALL);          //设置P0手动配置端口模式

    EnableP0nPullUp(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_3 | PIN_2); //使能P0.7,P0.6,P0.5,P0.4,P0.3,P0.2内部上拉电阻
    DisableP0nPullUp(PIN_1 | PIN_0);    //关闭P0.1,P0.0内部上拉电阻
    DisableP0nPullDown(PIN_ALL);        //关闭P0内部下拉电阻
    EnableP0nSchmitt(PIN_ALL);          //使能P0施密特触发
    SetP0nSlewRateNormal(PIN_6 | PIN_4 | PIN_1 | PIN_0); //设置P0.6,P0.4,P0.1,P0.0一般翻转速度
    SetP0nSlewRateFast(PIN_7 | PIN_5 | PIN_3 | PIN_2); //设置P0.7,P0.5,P0.3,P0.2快速翻转速度
    SetP0nDrivingNormal(PIN_ALL);       //设置P0一般驱动能力
    SetP0nDigitalInput(PIN_ALL);        //使能P0数字信号输入功能

    //<<AICUBE_USER_PORT0_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
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
    SetP1nQuasiMode(PIN_7 | PIN_6 | PIN_5 | PIN_3 | PIN_2); //设置P1.7,P1.6,P1.5,P1.3,P1.2为无弱上拉准双向口模式
    SetP1nHighZInputMode(PIN_4 | PIN_1 | PIN_0); //设置P1.4,P1.1,P1.0为高阻输入模式
    SetP1nManualMode(PIN_ALL);          //设置P1手动配置端口模式

    DisableP1nPullUp(PIN_ALL);          //关闭P1内部上拉电阻
    DisableP1nPullDown(PIN_ALL);        //关闭P1内部下拉电阻
    EnableP1nSchmitt(PIN_ALL);          //使能P1施密特触发
    SetP1nSlewRateNormal(PIN_ALL);      //设置P1一般翻转速度
    SetP1nDrivingNormal(PIN_ALL);       //设置P1一般驱动能力
    SetP1nDigitalInput(PIN_ALL);        //使能P1数字信号输入功能

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
    SetP2nQuasiMode(PIN_7 | PIN_6 | PIN_4 | PIN_1 | PIN_0); //设置P2.7,P2.6,P2.4,P2.1,P2.0为无弱上拉准双向口模式
    SetP2nPushPullMode(PIN_5 | PIN_3 | PIN_2); //设置P2.5,P2.3,P2.2为推挽输出模式
    SetP2nManualMode(PIN_ALL);          //设置P2手动配置端口模式

    EnableP2nPullUp(PIN_5 | PIN_4 | PIN_3 | PIN_2); //使能P2.5,P2.4,P2.3,P2.2内部上拉电阻
    DisableP2nPullUp(PIN_7 | PIN_6 | PIN_1 | PIN_0); //关闭P2.7,P2.6,P2.1,P2.0内部上拉电阻
    DisableP2nPullDown(PIN_ALL);        //关闭P2内部下拉电阻
    EnableP2nSchmitt(PIN_ALL);          //使能P2施密特触发
    SetP2nSlewRateNormal(PIN_7 | PIN_6 | PIN_4 | PIN_1 | PIN_0); //设置P2.7,P2.6,P2.4,P2.1,P2.0一般翻转速度
    SetP2nSlewRateFast(PIN_5 | PIN_3 | PIN_2); //设置P2.5,P2.3,P2.2快速翻转速度
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
    SetP3nManualMode(PIN_ALL);          //设置P3手动配置端口模式

    EnableP3nPullUp(PIN_3);             //使能P3.3内部上拉电阻
    DisableP3nPullUp(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_2 | PIN_1 | PIN_0); //关闭P3.7,P3.6,P3.5,P3.4,P3.2,P3.1,P3.0内部上拉电阻
    DisableP3nPullDown(PIN_ALL);        //关闭P3内部下拉电阻
    EnableP3nSchmitt(PIN_ALL);          //使能P3施密特触发
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
    SetP4nManualMode(PIN_ALL);          //设置P4手动配置端口模式

    DisableP4nPullUp(PIN_ALL);          //关闭P4内部上拉电阻
    DisableP4nPullDown(PIN_ALL);        //关闭P4内部下拉电阻
    EnableP4nSchmitt(PIN_ALL);          //使能P4施密特触发
    SetP4nSlewRateNormal(PIN_ALL);      //设置P4一般翻转速度
    SetP4nDrivingNormal(PIN_ALL);       //设置P4一般驱动能力
    SetP4nDigitalInput(PIN_ALL);        //使能P4数字信号输入功能

    SetP4nRisingInt(PIN_0);             //设置P4.0为上升沿中断

    EnableP4nInt(PIN_0);                //使能P4.0口中断

    SetP4IntPriority(0);                //设置中断为最低优先级

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
    SetP5nQuasiMode(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_3 | PIN_2); //设置P5.7,P5.6,P5.5,P5.4,P5.3,P5.2为无弱上拉准双向口模式
    SetP5nPushPullMode(PIN_1 | PIN_0);  //设置P5.1,P5.0为推挽输出模式
    SetP5nManualMode(PIN_ALL);          //设置P5手动配置端口模式

    EnableP5nPullUp(PIN_1 | PIN_0);     //使能P5.1,P5.0内部上拉电阻
    DisableP5nPullUp(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_3 | PIN_2); //关闭P5.7,P5.6,P5.5,P5.4,P5.3,P5.2内部上拉电阻
    DisableP5nPullDown(PIN_ALL);        //关闭P5内部下拉电阻
    EnableP5nSchmitt(PIN_ALL);          //使能P5施密特触发
    SetP5nSlewRateNormal(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_3 | PIN_2); //设置P5.7,P5.6,P5.5,P5.4,P5.3,P5.2一般翻转速度
    SetP5nSlewRateFast(PIN_1 | PIN_0);  //设置P5.1,P5.0快速翻转速度
    SetP5nDrivingNormal(PIN_ALL);       //设置P5一般驱动能力
    SetP5nDigitalInput(PIN_ALL);        //使能P5数字信号输入功能

    Port54DigitalToPin54();             //P5.4外部端口的数字功能为P5.4口相关的数字功能

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
    SetP6nQuasiMode(PIN_6 | PIN_3 | PIN_2 | PIN_1 | PIN_0); //设置P6.6,P6.3,P6.2,P6.1,P6.0为无弱上拉准双向口模式
    SetP6nPushPullMode(PIN_7 | PIN_5 | PIN_4); //设置P6.7,P6.5,P6.4为推挽输出模式
    SetP6nManualMode(PIN_ALL);          //设置P6手动配置端口模式

    EnableP6nPullUp(PIN_7 | PIN_6 | PIN_5 | PIN_4); //使能P6.7,P6.6,P6.5,P6.4内部上拉电阻
    DisableP6nPullUp(PIN_3 | PIN_2 | PIN_1 | PIN_0); //关闭P6.3,P6.2,P6.1,P6.0内部上拉电阻
    DisableP6nPullDown(PIN_ALL);        //关闭P6内部下拉电阻
    EnableP6nSchmitt(PIN_ALL);          //使能P6施密特触发
    SetP6nSlewRateNormal(PIN_6 | PIN_4 | PIN_3 | PIN_2 | PIN_1 | PIN_0); //设置P6.6,P6.4,P6.3,P6.2,P6.1,P6.0一般翻转速度
    SetP6nSlewRateFast(PIN_7 | PIN_5);  //设置P6.7,P6.5快速翻转速度
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
    SetP7nQuasiMode(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_2); //设置P7.7,P7.6,P7.5,P7.4,P7.2为无弱上拉准双向口模式
    SetP7nPushPullMode(PIN_3 | PIN_1 | PIN_0); //设置P7.3,P7.1,P7.0为推挽输出模式
    SetP7nManualMode(PIN_ALL);          //设置P7手动配置端口模式

    EnableP7nPullUp(PIN_3 | PIN_2 | PIN_1 | PIN_0); //使能P7.3,P7.2,P7.1,P7.0内部上拉电阻
    DisableP7nPullUp(PIN_7 | PIN_6 | PIN_5 | PIN_4); //关闭P7.7,P7.6,P7.5,P7.4内部上拉电阻
    DisableP7nPullDown(PIN_ALL);        //关闭P7内部下拉电阻
    EnableP7nSchmitt(PIN_ALL);          //使能P7施密特触发
    SetP7nSlewRateNormal(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_2 | PIN_0); //设置P7.7,P7.6,P7.5,P7.4,P7.2,P7.0一般翻转速度
    SetP7nSlewRateFast(PIN_3 | PIN_1);  //设置P7.3,P7.1快速翻转速度
    SetP7nDrivingNormal(PIN_ALL);       //设置P7一般驱动能力
    SetP7nDigitalInput(PIN_ALL);        //使能P7数字信号输入功能

    //<<AICUBE_USER_PORT7_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT7_INITIAL_END>>
}

void GPIO_Init(void){
    PORT0_Init();
    PORT1_Init();
    PORT2_Init();
    PORT3_Init();
    PORT4_Init();
    PORT5_Init();
    PORT6_Init();
    PORT7_Init();
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


