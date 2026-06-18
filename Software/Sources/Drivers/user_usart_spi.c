
#include "config.h"

void SPI_Init()
{
    P2M1 &= ~0x7c;  //SPI接口设置成推挽输出
    P2M0 |= 0x7c;
    
    P2SR &= ~0x28;  //P2.3(MOSI) P2.5(SCLK)设置成高速模式
 //   P2PU |= 0x54;   //SPI接口使能上拉电阻

    // P_LCD_CLK = 1;  //SCLK
    // P_LCD_SDA = 1;  //MOSI
    // P_LCD_RST = 1;  //RES
    // P_LCD_DC  = 1;  //DC
    // P_LCD_CS  = 1;  //CS

    S1SPI_S1 = 0;   //00: P1.2 P1.3 P1.4 P1.5, 01: P2.2 P2.3 P2.4 P2.5, 10: P5.4 P4.0 P4.1 P4.3, 11: P3.5 P3.4 P3.3 P3.2
    S1SPI_S0 = 1;   //切换S1SPI到P2.2/S1SS,P2.3/S1MOSI,P2.4/S1MISO,P2.5/S1SCLK
    SCON = 0x10;    //使能接收,必须设置为串口模式0
    
    USARTCR1 = 0x10;        //使能USART1的SPI模式
//  USARTCR1 |= 0x40;       //DORD=1
    USARTCR1 &= ~0x40;      //DORD=0
//  USARTCR1 |= 0x04;       //从机模式
    USARTCR1 &= ~0x04;      //主机模式
    USARTCR1 |= 0x00;       //CPOL=0, CPHA=0
//  USARTCR1 |= 0x01;       //CPOL=0, CPHA=1
//  USARTCR1 |= 0x02 ;      //CPOL=1, CPHA=0
//  USARTCR1 |= 0x03;       //CPOL=1, CPHA=1
    USARTCR4 = 0x00;        //SPI速度为SYSCLK/4
//  USARTCR4 = 0x01;        //SPI速度为SYSCLK/8
//  USARTCR4 = 0x02;        //SPI速度为SYSCLK/16
//  USARTCR4 = 0x03;        //SPI速度为SYSCLK/2
    
//  USARTCR4 |= 0x10;       //MOSI/MISO swap
    USARTCR1 |= 0x08;       //使能SPI功能
}