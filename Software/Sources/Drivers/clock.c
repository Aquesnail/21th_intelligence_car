//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: clock.c
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

void CLK_Init(void)
{
    CLK_I2S1CLK_Divider(1);             //设置I2S1时钟预分频系数
    CLK_I2S2CLK_Divider(1);             //设置I2S2时钟预分频系数
    CLK_SPI1CLK_Divider(1);             //设置SPI1时钟预分频系数
    CLK_SPI2CLK_Divider(1);             //设置SPI2时钟预分频系数
    CLK_SPI3CLK_Divider(1);             //设置SPI3时钟预分频系数
    CLK_PWMACLK_Divider(7);             //设置PWMA时钟预分频系数
    CLK_PWMBCLK_Divider(1);             //设置PWMB时钟预分频系数
    CLK_PWMCCLK_Divider(1);             //设置PWMC时钟预分频系数
    CLK_PWMDCLK_Divider(1);             //设置PWMD时钟预分频系数
    CLK_PWMECLK_Divider(70);            //设置PWME时钟预分频系数
    CLK_PWMFCLK_Divider(1);             //设置PWMF时钟预分频系数
    CLK_TFPUCLK_Divider(1);             //设置TFPU时钟预分频系数

    CLK_IRC48M_Enable();                //启动内部48M高速IRC
    CLK_IRC48M_WaitStable();            //等待振荡器稳定

    CLK_SYSCLK_Divider(10);             //切换主时钟前先将系统时钟降频

    HIRC_48M();                         //选择内部预置的频率

    CLK_MCLK_HIRC();                    //选择内部高精度HIRC作为主时钟

    CLK_HPLL1_IRC48M();                 //选择内部48M高速IRC作为HPLL1输入时钟
    CLK_HPLL1_PreDivider(8);            //设置HPLL1的输入时钟预分频系数(分频后的频率必须为6MHz左右)
    CLK_HPLL1_Lock420MHz();             //选择HPLL1的锁频目标频率
    CLK_HPLL1_Enable();                 //启动HPLL1

    CLK_HPLL2_IRC48M();                 //选择内部48M高速IRC作为HPLL2输入时钟
    CLK_HPLL2_PreDivider(8);            //设置HPLL2的输入时钟预分频系数(分频后的频率必须为6MHz左右)
    CLK_HPLL2_Lock480MHz();             //选择HPLL2的锁频目标频率
    CLK_HPLL2_Enable();                 //启动HPLL2
    delay_ms(1);                        //等待PLL锁频

    CLK_MCLK2_HPLL2D2();                //选择HPLL2的输出2分频时钟作为主时钟

    CLK_SYSCLK_Divider(2);              //设置系统时钟分频系数

    CLK_MCLKO_NONE();                   //关闭主时钟输出功能

    CLK_HSI2SCK_HPLL1D2();              //选择HPLL1的输出2分频时钟作为高速I2S时钟源
    CLK_HSIOCK_HPLL1D2();               //选择HPLL1的输出2分频时钟作为高速外设时钟源

    //<<AICUBE_USER_CLOCK_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_CLOCK_INITIAL_END>>
}

////////////////////////////////////////
// 时钟初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
// void CLK_Init(void)
// {
//    // P1M0 = 0x00;
//    // P1M1 = PIN_0 | PIN_1 | PIN_2 | PIN_3 | PIN_6 | PIN_7;

    
//     CLK_I2S1CLK_Divider(1);             //设置I2S1时钟预分频系数
//     CLK_I2S2CLK_Divider(1);             //设置I2S2时钟预分频系数
//     CLK_SPI1CLK_Divider(1);             //设置SPI1时钟预分频系数
//     CLK_SPI2CLK_Divider(1);             //设置SPI2时钟预分频系数
//     CLK_SPI3CLK_Divider(1);             //设置SPI3时钟预分频系数
//     CLK_PWMACLK_Divider(7);             //设置PWMA时钟预分频系数
//     CLK_PWMBCLK_Divider(1);             //设置PWMB时钟预分频系数
//     CLK_PWMCCLK_Divider(1);             //设置PWMC时钟预分频系数
//     CLK_PWMDCLK_Divider(1);             //设置PWMD时钟预分频系数
//     CLK_PWMECLK_Divider(70);            //设置PWME时钟预分频系数
//     CLK_PWMFCLK_Divider(1);             //设置PWMF时钟预分频系数
//     CLK_TFPUCLK_Divider(1);             //设置TFPU时钟预分频系数

//     CLK_XTAL_Enable();                  //启动外部高速无源晶振
//     CLK_XOSC_LowGain();                 //振荡器低增益
//     CLK_XOSC_WaitStable();              //等待振荡器稳定

//     CLK_IRC48M_Enable();                //启动内部48M高速IRC
//     CLK_IRC48M_WaitStable();            //等待振荡器稳定

//     CLK_SYSCLK_Divider(10);             //切换主时钟前先将系统时钟降频

//     HIRC_48M();                         //选择内部预置的频率

//     CLK_MCLK_XTAL();                    //选择外部高速晶振作为主时钟

//     CLK_HPLL1_BASECLK();                //选择内部基本时钟BASECLK作为HPLL1输入时钟
//     CLK_HPLL1_PreDivider(4);            //设置HPLL1的输入时钟预分频系数(分频后的频率必须为6MHz左右)
//     CLK_HPLL1_Lock420MHz();             //选择HPLL1的锁频目标频率
//     CLK_HPLL1_Enable();                 //启动HPLL1

//     CLK_HPLL2_IRC48M();                 //选择内部48M高速IRC作为HPLL2输入时钟
//     CLK_HPLL2_PreDivider(8);            //设置HPLL2的输入时钟预分频系数(分频后的频率必须为6MHz左右)
//     CLK_HPLL2_Lock480MHz();             //选择HPLL2的锁频目标频率
//     CLK_HPLL2_Enable();                 //启动HPLL2
//     delay_ms(1);                        //等待PLL锁频

//     CLK_MCLK2_HPLL2D2();                //选择HPLL2的输出2分频时钟作为主时钟

//     CLK_SYSCLK_Divider(2);              //设置系统时钟分频系数

//     CLK_MCLKO_NONE();                   //关闭主时钟输出功能

//     CLK_HSI2SCK_HPLL1D2();              //选择HPLL1的输出2分频时钟作为高速I2S时钟源
//     CLK_HSIOCK_HPLL1D2();               //选择HPLL1的输出2分频时钟作为高速外设时钟源

//     //<<AICUBE_USER_CLOCK_INITIAL_BEGIN>>
//     // 在此添加用户初始化代码  
//     //<<AICUBE_USER_CLOCK_INITIAL_END>>
// }



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


