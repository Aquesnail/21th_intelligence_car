/*---------------------------------------------------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/*---------------------------------------------------------------------*/

/*************  功能说明    **************

本例程基于STC32G144K246为主控芯片的实验箱进行编写测试。

使用Keil C251编译器，Memory Model推荐设置XSmall模式，默认定义变量在edata，单时钟存取访问速度快。

edata建议保留1K给堆栈使用，空间不够时可将大数组、不常用变量加xdata关键字定义到xdata空间。

本实验测试使用的是1.3寸240*240 USART-SPI接口驱动的彩屏

通过DMA硬件自动将xdata空间数据传输给USART-SPI接口，在LCD屏幕显示。

下载时, 选择IRC频率 48MHz，主时钟使用 PLL 90MHz 时钟频率.

******************************************/

#include "tft.h"
#include "pic.h"

/****************  用户定义宏  ****************/


/****************  本地常量声明  **************/


/****************  本地变量声明  **************/


/****************  本地函数声明  **************/

void HPLL_config(void);
void SPI_init(void);
void usart1_tx_dma(u16 size, u8 xdata *pdat);

/******************** 主函数 **************************/
void main(void)
{
    EAXFR = 1; //扩展寄存器(XFR)访问使能
    CKCON = 0; //提高访问XRAM速度

    HPLL_config();
    SPI_Init();
    LCD_Init();
    usart1_tx_dma(64800-1,IMG_DATA);
    EA = 1;     //允许总中断

    LCD_Fill(0,0,240,240,LGRAY);
    TFT_ShowStart();
    while (1);
}

//========================================================================
// 函数: void HPLL_config(void)
// 描述: PLL时钟配置函数。
// 参数: none.
// 返回: none.
// 版本: VER1.0
// 日期: 2025-11-01
// 备注: 
//========================================================================
void HPLL_config(void)
{
#if(HPLL_SEL==SEL_HPLL2)
    WTST = 2;           //通过WTST增加等待时钟控制Flash读取速度在33MHz以内, 主时钟:360MHz/2/2=90MHz, 读Flash速度:90MHz/(1+2)=30MHz

    HPLLCR |= 0x80;     //使能HPLL（高速外设默认使用HPLL时钟，切换到HPLL2前要先使能HPLL，切换完再关闭HPLL）

                        //首先需要将HIRC主频调节到48MHz
    HPLL2CR &= ~0x10;   //选择HPLL2输入时钟源为HIRC
//    HPLL2CR |= 0x10;    //选择HPLL2输入时钟源为IRCM
    HPLL2PDIV = 8;      //设置HPLL2输入时钟预分频为8（HPLL输入频率必须为6MHz）
//    HPLL2CR |= 0x00;    //HPLL2=6MHz*52=312MHz
//    HPLL2CR |= 0x01;    //HPLL2=6MHz*54=324MHz
//    HPLL2CR |= 0x02;    //HPLL2=6MHz*56=336MHz
//    HPLL2CR |= 0x03;    //HPLL2=6MHz*58=348MHz
    HPLL2CR |= 0x04;    //HPLL2=6MHz*60=360MHz
//    HPLL2CR |= 0x05;    //HPLL2=6MHz*62=372MHz
//    HPLL2CR |= 0x06;    //HPLL2=6MHz*64=384MHz
//    HPLL2CR |= 0x07;    //HPLL2=6MHz*66=396MHz
//    HPLL2CR |= 0x08;    //HPLL2=6MHz*68=408MHz
//    HPLL2CR |= 0x09;    //HPLL2=6MHz*70=420MHz
//    HPLL2CR |= 0x0a;    //HPLL2=6MHz*72=432MHz
//    HPLL2CR |= 0x0b;    //HPLL2=6MHz*74=444MHz
//    HPLL2CR |= 0x0c;    //HPLL2=6MHz*76=456MHz
//    HPLL2CR |= 0x0d;    //HPLL2=6MHz*78=468MHz
//    HPLL2CR |= 0x0e;    //HPLL2=6MHz*80=480MHz
//    HPLL2CR |= 0x0f;    //HPLL2=6MHz*82=492MHz
    HPLL2CR |= 0x20;    //高速外设时钟选择 HPLL2/2
    HPLL2CR |= 0x80;    //使能HPLL2

    HPLLCR &= ~0x80;    //关闭HPLL（高速外设默认使用HPLL时钟，切换到HPLL2前要先使能HPLL，切换完再关闭HPLL）
    
    CLKDIV = 2;         //系统时钟 = 主时钟源/2 = HPLL2/2/2 = 90MHz
    CLKSEL = 0x08;      //选择HPLL2/2作为主时钟源

#else
    WTST = 3;           //通过WTST增加等待时钟控制Flash读取速度在33MHz以内, 主时钟:480MHz/2/2=120MHz, 读Flash速度:120MHz/(1+3)=30MHz
                        //首先需要将HIRC主频调节到48MHz
    HPLLCR &= ~0x10;    //选择HPLL输入时钟源为HIRC
//    HPLLCR |= 0x10;     //选择HPLL输入时钟源为IRCM
    HPLLPDIV = 8;       //设置HPLL输入时钟预分频为8（HPLL输入频率必须为6MHz）
//    HPLLCR |= 0x00;     //HPLL=6MHz*52=312MHz
//    HPLLCR |= 0x01;     //HPLL=6MHz*54=324MHz
//    HPLLCR |= 0x02;     //HPLL=6MHz*56=336MHz
//    HPLLCR |= 0x03;     //HPLL=6MHz*58=348MHz
//    HPLLCR |= 0x04;     //HPLL=6MHz*60=360MHz
//    HPLLCR |= 0x05;     //HPLL=6MHz*62=372MHz
//    HPLLCR |= 0x06;     //HPLL=6MHz*64=384MHz
//    HPLLCR |= 0x07;     //HPLL=6MHz*66=396MHz
//    HPLLCR |= 0x08;     //HPLL=6MHz*68=408MHz
//    HPLLCR |= 0x09;     //HPLL=6MHz*70=420MHz
//    HPLLCR |= 0x0a;     //HPLL=6MHz*72=432MHz
//    HPLLCR |= 0x0b;     //HPLL=6MHz*74=444MHz
//    HPLLCR |= 0x0c;     //HPLL=6MHz*76=456MHz
//    HPLLCR |= 0x0d;     //HPLL=6MHz*78=468MHz
    HPLLCR |= 0x0e;     //HPLL=6MHz*80=480MHz
//    HPLLCR |= 0x0f;     //HPLL=6MHz*82=492MHz
    HPLL2CR &= ~0x60;   //高速外设时钟选择 HPLL/2
    HPLLCR |= 0x80;     //使能HPLL

    CLKDIV = 2;         //系统时钟=主时钟源/2 = HPLL/2/2 = 120MHz
    CLKSEL = 0x04;      //选择HPLL/2作为主时钟源
#endif

//    P5M0 |= 0x10; P5M1 &= ~0x10;//设置P5.4口为推挽输出
//    P5SR &= ~0x10;              //设置P5.4口为快速模式
//    HIRCCR |= 0x10;             //输出系统时钟
//    MCLKOCR = 10;               //系统时钟 10 分频到P5.4
}

//========================================================================
// 函数: void DMA_Config(void)
// 描述: USART DMA 功能配置.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2021-5-6
//========================================================================
void usart1_tx_dma(u16 size, u8 xdata *pdat)
{
    size--;                             //DMA传输字节数比实际少1
    
    DMA_UR1T_STA = 0x00;                //清除DMA状态
    DMA_UR1T_CFG = 0x80;                //开启DMA中断
    DMA_UR1T_AMT = size;                //设置DMA传输字节数
    DMA_UR1T_AMTH = size >> 8;
    DMA_UR1T_TXAL = (u8)pdat;         //设置缓冲区地址(注意:缓冲区必须是xdata类型)
    DMA_UR1T_TXAH = (u16)pdat >> 8;
    DMA_UR1T_CR = 0x80;                 //使能DMA
    
//    while (!(DMA_UR1T_STA & 0x01));     //等待DMA数据传输完成
//    DMA_UR1T_STA = 0x00;                //清除DMA状态
//    DMA_UR1T_CR = 0x00;                 //关闭DMA
}

void usart1_rx_dma(u16 size, u8 xdata *pdat)
{
    size--;                             //DMA传输字节数比实际少1
    
    DMA_UR1R_STA = 0x00;                //清除DMA状态
    DMA_UR1R_CFG = 0x00;                //关闭DMA中断
    DMA_UR1R_AMT = size;                //设置DMA传输字节数
    DMA_UR1R_AMTH = size >> 8;
    DMA_UR1R_RXAL = (u8)pdat;         //设置缓冲区地址(注意:缓冲区必须是xdata类型)
    DMA_UR1R_RXAH = (u16)pdat >> 8;
    DMA_UR1R_CR = 0xa1;                 //使能DMA,清空接收FIFO,触发串口1接收数据
    
                                        //!!!!!!!!!!!!!
    usart1_tx_dma(size+1, pdat);        //注意:接收数据时必须同时启动发送DMA
                                        //!!!!!!!!!!!!!
    
    while (!(DMA_UR1R_STA & 0x01));     //等待DMA数据传输完成
    DMA_UR1R_STA = 0x00;                //清除DMA状态
    DMA_UR1R_CR = 0x00;                 //关闭DMA
}

//========================================================================
// 函数: void UR1_DMA_Interrupt (void) interrupt 50/51
// 描述: UART1 DMA中断函数
// 参数: none.
// 返回: none.
// 版本: VER1.0
// 日期: 2025-12-26
// 备注: 
//========================================================================
void UR1T_DMA_Interrupt(void) interrupt DMA_UR1T_VECTOR
{
    DMA_UR1T_STA = 0x00;                //清除DMA状态
    TFT_ShowEnd();
}

void UR1R_DMA_Interrupt(void) interrupt DMA_UR1R_VECTOR
{
    DMA_UR1R_STA = 0x00;                //清除DMA状态
}
