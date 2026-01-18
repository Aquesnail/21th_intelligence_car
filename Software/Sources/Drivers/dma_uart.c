#include "stc32g144k246_def.h"

#include "dma_uart.h"
#include "ringbuffer.h"

static RingBuffer uartTxRingBufferObject;
static RingBuffer uartRxRingBufferObject;


static uint8_t uartDMATxBuffer[UART_DMATxBufferLength];
static uint8_t uartDMARxBuffer[UART_DMARxBufferLength];
static uint8_t uartTxRingBuffer[UART_TxRingBufferLength];
static uint8_t uartRxRingBuffer[UART_RxRingBufferLength];

#define _UART_DMA_GET_RX ((uint16_t)DMA_UR5R_DONEH << 8)+(DMA_UR5R_DONE)
#define _UART_DMA_

#define Baudrate 115200



void UART5_Timer2_Config(uint16_t dat);







void UART5_Config(uint8_t brt)    // 选择波特率, 2: 使用Timer2做波特率, 其它值: 使用Timer5做波特率.

{

    if(brt == 2)

    {

        S5CFG &= ~0x01; //串口5选择定时器2为波特率发生器

        UART5_Timer2_Config((u16)(65536UL - (MAIN_Fosc / 4) / Baudrate));

        S5CON = 0x50;   //8位数据, 允许接收

    }

    else

    {

        S5CFG |= 0x01;  //串口5选择定时器5为波特率发生器

        S5CON = 0x50;   //8位数据, 允许接收

        TH5 = (65536UL - (MAIN_Fosc / 4) / Baudrate) / 256;

        TL5 = (65536UL - (MAIN_Fosc / 4) / Baudrate) % 256;

        T6T5M &= ~0x0f;

        T6T5M |= 0x0a;

    }

//    S5INTR |= 0x80;     //允许UART5中断

   //UART5_SW(UART5_SW_P04_P05);     //UART5_SW_P04_P05,UART5_SW_P40_P41,UART5_SW_P14_P15,UART5_SW_P20_P21

    UART5_SwitchP2021();



    UR5TOCR = 0xc0;     //bit7:使能超时接收，bit6:使能超时中断，bit5:超时时钟选择 1:系统时钟  0:串口数据位率(波特率)

    UR5TOTL = 0x40;

    UR5TOTH = 0x00;

    UR5TOTE = 0x00;     //需要写 UR5TOTE 后，新的TO值才会生效

}



void UART5_Timer2_Config(uint16_t dat){

    T2R = 0;    //Timer stop

    T2_CT = 0;  //Timer2 set As Timer

    T2x12 = 1;  //Timer2 set as 1T mode

    T2H = dat / 256;

    T2L = dat % 256;

    ET2 = 0;    //禁止中断

    T2R = 1;    //Timer run enable

}



void UART5_DMA_Config(void)

{

    //DMA的xdata区域选择(switch by DMA_ARB_CFG[6:5])

//    DMA_ARB_CFG &= ~0x60;   //00: lower 64K(01:0000H ~ 01:FFFFH), (default)

//    DMA_ARB_CFG |= 0x20;    //01: upper 64K(02:0000H ~ 02:FFFFH)

//    DMA_ARB_CFG |= 0x40;    //10: 4K exe ram(03:0000H ~ 03:0FFFH)



    DMA_UR5T_CFG = 0x80;        //bit7 1:Enable Interrupt

    DMA_UR5T_STA = 0x00;

    DMA_UR5T_AMT = 0xff;        //设置传输总字节数(低8位)：n+1

    DMA_UR5T_AMTH = 0x00;       //设置传输总字节数(高8位)：n+1

    DMA_UR5T_TXAH = (u8)((u16)&uartDMATxBuffer >> 8);

    DMA_UR5T_TXAL = (u8)((u16)&uartDMATxBuffer);

    DMA_UR5T_CR = 0xc0;         //bit7 1:使能 UART5_DMA, bit6 1:开始 UART5_DMA 自动发送



    DMA_UR5R_CFG = 0x80;        //bit7 1:Enable Interrupt

    DMA_UR5R_STA = 0x00;

    DMA_UR5R_AMT = 0xff;        //设置传输总字节数(低8位)：n+1

    DMA_UR5R_AMTH = 0x00;       //设置传输总字节数(高8位)：n+1

    DMA_UR5R_RXAH = (u8)((u16)&uartDMARxBuffer >> 8);

    DMA_UR5R_RXAL = (u8)((u16)&uartDMARxBuffer);

    DMA_UR5R_CR = 0xa1;         //bit7 1:使能 UART5_DMA, bit5 1:开始 UART5_DMA 自动接收, bit0 1:清除 FIFO





    //中断优先级调整

    DMA_UART5_SetRxBusPriority(0);

    DMA_UART6_SetRxIntPriority(0);

    DMA_UART5_SetTxBusPriority(0);

    DMA_UART6_SetTxIntPriority(0); //全部为最低优先级



}



void UART5_Init(void){

    UART5_Config(2); //参数2表示使用timer2

    UART5_DMA_Config();

   

}



void UART5_ISR(void) interrupt UART5_VECTOR

{

    if (UART5_CheckTxFlag())            //判断串口发送中断

    {

        UART5_ClearTxFlag();            //清除串口发送中断标志

    }



    if (UART5_CheckRxFlag())            //判断串口接收中断

    {

        UART5_ClearRxFlag();            //清除串口接收中断标志

    }



    if (UART5_CheckTimeoutFlag())       //判断串口接收超时中断

    {

        UART5_ClearTimeoutFlag();       //清除串口接收超时中断标志

    }

}



void UART5_DMATx_ISR(void) interrupt DMA_UR5T_VECTOR

{

     if (DMA_UART5_CheckTxFlag())        //判断串口发送DMA中断

    {

        DMA_UART5_ClearTxFlag();        //清除串口发送DMA中断标志

    }

}



void UART5_DMARx_ISR(void) interrupt DMA_UR5R_VECTOR

{

    if (DMA_UART5_CheckRxFlag())        //判断串口接收DMA中断

    {

        DMA_UART5_ClearRxFlag();        //清除串口接收DMA中断标志

    }

}



void UART5Putc(char dat){

    S5BUF = dat;

    while((S5CON & 2)== 0);

    S5CON &= ~0x02;

} //普通收发函数

void UART5_RingBuffer_Init(void){
    rb_init(uartTxRingBuffer,&uartTxRingBufferObject,UART_TxRingBufferLength);
    rb_init(uartRxRingBuffer,&uartTxRingBufferObject,UART_RxRingBufferLength);
}
void UART5_TxingBuffer_Clear(void){
    rb_clear(&uartTxRingBufferObject);
}
uint8_t UART5_TxRingBuffer_IsEmpty(void){
    return rb_is_empty(&uartTxRingBufferObject);
}
uint8_t UART5_TxRingBuffer_IsFull(void){
    return rb_is_full(&uartTxRingBufferObject);
}
uint32_t UART5_TxRingBUffer_Available(void){
    return rb_available(&uartTxRingBufferObject);
}
void UART5_RxingBuffer_Clear(void){
    rb_clear(&uartRxRingBufferObject);
}
uint8_t UART5_RxRingBuffer_IsEmpty(void){
    return rb_is_empty(&uartRxRingBufferObject);
}
uint8_t UART5_RxRingBuffer_IsFull(void){
    return rb_is_full(&uartRxRingBufferObject);
}
uint32_t UART5_RxRingBUffer_Available(void){
    return rb_available(&uartRxRingBufferObject);
}
static void UART5_DMA_Send(uint16_t len){
    len--;
    DMA_UR5R_CR = 0;            //关闭DMA接收
    DMA_UR5T_AMT = (uint8_t)(len);       //设置传输总字节数(低8位)：n+1
    DMA_UR5T_AMTH = (uint8_t)(len>>8); //设置传输总字节数(高8位)：n+1

    DMA_UR5T_CR = 0xc0;         //bit7 1:使能 UART5_DMA, bit6 1:开始 UART5_DMA 自动发送
    DMA_UR5R_CR = 0xa1;         //bit7 1:使能 UART5_DMA, bit5 1:开始 UART5_DMA 自动接收, bit0 1:清除 FIFO
}   

uint16_t UART5_Write(char* dat,uint16_t len){
    uint8_t i;
    for(i=0;i<len;i++){
        if(uartTxRingBufferObject.head==uartTxRingBufferObject.tail){
            return 0;
        }
        dat[i]=uartTxRingBufferObject.pBuffer[uartTxRingBufferObject.tail];
        uartTxRingBufferObject.tail=(uartTxRingBufferObject.tail+1)%UART_TxRingBufferLength;
    }
    return 0;
}