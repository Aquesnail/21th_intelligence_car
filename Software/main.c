/*---------------------------------------------------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/*---------------------------------------------------------------------*/

/*************  功能说明    **************

本例程基于STC32G144K246为主控芯片的实验箱进行编写测试。

使用Keil C251编译器，Memory Model推荐设置XSmall模式，默认定义变量在edata，单时钟存取访问速度快。

edata建议保留1K给堆栈使用，空间不够时可将大数组、不常用变量加xdata关键字定义到xdata空间。

通过串口对PM25LV040/W25X40CL/W25Q80BV/W25Q64BV/W25Q128FV进行读写测试。

对FLASH做扇区擦除、写入、读出的操作，命令指定地址。

通过实验箱USB2接口的CDC1 COM与电脑串口助手通信，默认波特率:  115200,8,N,1. 

串口命令设置: (字母不区分大小写)
    E 0x001234              --> 对EEPROM进行扇区擦除操作，指定十六进制地址.
    W 0x001234 1234567890   --> 对EEPROM进行扇区写入操作，指定十六进制地址，后面为写入内容.
    R 0x001234 10           --> 对EEPROM进行扇区读出操作，指定十六进制地址. 
    C                       --> 如果检测不到SPI Flash, 发送C强制允许操作.

注意：为了通用，程序不识别地址是否有效，用户自己根据具体的型号来决定。

下载时, 选择时钟 48MHz (用户可自行修改频率).

******************************************/

#include "..\comm\STC32G144K246.h"

typedef 	unsigned char	u8;
typedef 	unsigned int	u16;
typedef 	unsigned long	u32;

/****************  用户定义宏  ****************/

#define MAIN_Fosc           48000000UL
#define Baudrate            115200L
#define TM                  (65536 -(MAIN_Fosc/Baudrate/4))

#define EE_BUF_LENGTH       50

#define UART_BUF_LENGTH    (EE_BUF_LENGTH+9)   //串口缓冲长度

/****************  本地常量声明  **************/


/****************  本地变量声明  **************/

u8  tmp[EE_BUF_LENGTH];
u8  sst_byte;
u32 Flash_addr;

/*************  FLASH相关变量声明   **************/

sbit    FLASH_CE  = P7^5;     //PIN1
sbit    FLASH_SO  = P7^2;     //PIN2
sbit    FLASH_SI  = P7^0;     //PIN5
sbit    FLASH_SCK = P7^1;     //PIN6

sbit    FLASH_WP  = P7^3;     //PIN3
sbit    FLASH_HOLD= P7^4;     //PIN7

u8  B_FlashOK;      //Flash状态
u8  FLASH_ID, FLASH_ID1, FLASH_ID2;

bit B_TX1_Busy;     //发送忙标志
bit B_RX1_TimeOut;  //接收超时标志
u8  TX1_Cnt;        //发送计数
u8  RX1_Cnt;        //接收计数

u8  RX1_Buffer[UART_BUF_LENGTH];   //接收缓冲

/****************  本地函数声明  **************/

void UartInit(void);
void RX1_Check(void);
void UartPutc(unsigned char dat);

void SPI_init(void);
void FlashCheckID(void);
u8 CheckFlashBusy(void);
void FlashWriteEnable(void);
void FlashChipErase(void);
void FlashSectorErase(u32 addr);
void SPI_Read_Nbytes( u32 addr, u8 *buffer, u16 size);
u8 SPI_Read_Compare(u32 addr, u8 *buffer, u16 size);
void SPI_Write_Nbytes(u32 addr, u8 *buffer,  u8 size);

/********************* 主函数 *************************/
void main(void)
{
    EAXFR = 1; //扩展寄存器(XFR)访问使能
    CKCON = 0; //提高访问XRAM速度

    P3M1 = 0x40;   P3M0 = 0x80;   //P36设置高阻输入,P37设置推挽输出
    P7M1 = 0x04;   P7M0 = 0x3b;   //P72设置高阻输入,P70,P71,P73~P75设置推挽输出

    P3PU = 0x40;    //P36口使能内部上拉电阻
    P7PU = 0x04;    //P72口使能内部上拉电阻

    UartInit();     //串口初始化
    
    EA = 1;         //打开总中断
    
    printf("命令设置:\r\n");
    printf("E 0x001234            --> 擦除\xfd扇区  十六进制地址\r\n");   //FD编码汉字后面需要添加"\xfd"，避免乱码
    printf("W 0x001234 1234567890 --> 写入操作  十六进制地址  写入内容\r\n");
    printf("R 0x001234 10         --> 读出操作  十六进制地址  读出字节\r\n");
    printf("C                     --> 如果检测不到SPI Flash, 发送C强制允许操作.\r\n\r\n");

    SPI_init();
    FlashCheckID();
    FlashCheckID();
    
    if(!B_FlashOK)  printf("未检测到PM25LV040/W25X40CL/W25Q80BV/W25Q64BV/W25Q128FV!\r\n");
    else
    {
        if(B_FlashOK == 1)
        {
            printf("检测到PM25LV040!\r\n");
        }
        else if(B_FlashOK == 2)
        {
            printf("检测到W25X40CL!\r\n");
        }
        else if(B_FlashOK == 3)
        {
            printf("检测到W25Q80BV!\r\n");
        }
        else if(B_FlashOK == 4)
        {
            printf("检测到W25Q64BV!\r\n");
        }
        else if(B_FlashOK == 5)
        {
            printf("检测到W25Q128FV!\r\n");
        }
    }
    printf("制造商ID1 = 0x%02X",FLASH_ID1);
    printf("\r\n      ID2 = 0x%02X",FLASH_ID2);
    printf("\r\n   设备ID = 0x%02X\r\n",FLASH_ID);

    while(1)
    {
        if(B_RX1_TimeOut)
        {
            B_RX1_TimeOut = 0;

            if(RX1_Cnt > 0)
            {
                RX1_Check();    //串口1处理数据
            }
            RX1_Cnt = 0;
        }
    }
}

/**************** ASCII码转BIN ****************************/
u8  CheckData(u8 dat)
{
    if((dat >= '0') && (dat <= '9'))        return (dat-'0');
    if((dat >= 'A') && (dat <= 'F'))        return (dat-'A'+10);
    return 0xff;
}

/**************** 获取写入地址 ****************************/
u32 GetAddress(void)
{
    u32 address;
    u8  i,j;
    
    address = 0;
    if((RX1_Buffer[2] == '0') && (RX1_Buffer[3] == 'X'))
    {
        for(i=4; i<10; i++)
        {
            j = CheckData(RX1_Buffer[i]);
            if(j >= 0x10)   return 0x80000000;  //error
            address = (address << 4) + j;
        }
        return (address);
    }
    return  0x80000000; //error
}

/**************** 获取要读出数据的字节数 ****************************/
u8  GetDataLength(void)
{
    u8  i;
    u8  length;
    
    length = 0;
    for(i=11; i<RX1_Cnt; i++)
    {
        if(CheckData(RX1_Buffer[i]) >= 10)  break;
        length = length * 10 + CheckData(RX1_Buffer[i]);
    }
    return (length);
}

/**************** 串口处理函数 ****************************/

void RX1_Check(void)
{
    u8  i,j;

    if((RX1_Cnt == 1) && (RX1_Buffer[0] == 'C'))    //发送C强制允许操作
    {
        B_FlashOK = 1;
        printf("强制允许操作FLASH!\r\n");
    }

    if(!B_FlashOK)
    {
        printf("PM25LV040/W25X40CL/W25Q80BV/W25Q64BV/W25Q128FV不存在, 不能操作FLASH!\r\n");
        return;
    }
    
    F0 = 0;
    if((RX1_Cnt >= 10) && (RX1_Buffer[1] == ' '))   //最短命令为10个字节
    {
        for(i=0; i<10; i++)
        {
            if((RX1_Buffer[i] >= 'a') && (RX1_Buffer[i] <= 'z'))    RX1_Buffer[i] = RX1_Buffer[i] - 'a' + 'A';//小写转大写
        }
        Flash_addr = GetAddress();
        if(Flash_addr < 0x80000000)
        {
            if(RX1_Buffer[0] == 'E')    //擦除
            {
                FlashSectorErase(Flash_addr);
                printf("已擦除\xfd当前扇区内容!\r\n");
                F0 = 1;
            }

            else if((RX1_Buffer[0] == 'W') && (RX1_Cnt >= 12) && (RX1_Buffer[10] == ' '))   //写入N个字节
            {
                j = RX1_Cnt - 11;
                for(i=0; i<j; i++)  tmp[i] = 0xff;      //检测要写入的空间是否为空
                i = SPI_Read_Compare(Flash_addr,tmp,j);
                if(i > 0)
                {
                    printf("要写入的地址为非空,不能写入,请先擦掉!\r\n");
                }
                else
                {
                    SPI_Write_Nbytes(Flash_addr,&RX1_Buffer[11],j);     //写N个字节 
                    i = SPI_Read_Compare(Flash_addr,&RX1_Buffer[11],j); //比较写入的数据
                    if(i == 0)
                    {
                        printf("已写入");
                        if(j >= 100)    {UartPutc((u8)(j/100+'0'));   j = j % 100;}
                        if(j >= 10)     {UartPutc((u8)(j/10+'0'));    j = j % 10;}
                        UartPutc((u8)(j%10+'0'));
                        printf("字节内容!\r\n");
                    }
                    else printf("写入错误!\r\n");
                }
                F0 = 1;
            }
            else if((RX1_Buffer[0] == 'R') && (RX1_Cnt >= 12) && (RX1_Buffer[10] == ' '))   //读出N个字节
            {
                j = GetDataLength();
                if((j > 0) && (j < EE_BUF_LENGTH))
                {
                    SPI_Read_Nbytes(Flash_addr,tmp,j);
                    printf("读出");
                    if(j>=100)  UartPutc((u8)(j/100+'0'));
                    UartPutc((u8)(j%100/10+'0'));
                    UartPutc((u8)(j%10+'0'));
                    printf("个字节内容如下：\r\n");
                    for(i=0; i<j; i++)  UartPutc(tmp[i]);
                    UartPutc(0x0d);
                    UartPutc(0x0a);
                    F0 = 1;
                }
            }
        }
    }
    if(!F0)
    {
        printf("命令设置:\r\n");
        printf("E 0x001234            --> 擦除\xfd扇区  十六进制地址\r\n");
        printf("W 0x001234 1234567890 --> 写入操作  十六进制地址  写入内容\r\n");
        printf("R 0x001234 10         --> 读出操作  十六进制地址  读出字节\r\n");
        printf("C                     --> 如果检测不到SPI Flash, 发送C强制允许操作.\r\n\r\n");
    }
}

/******************* FLASH相关程序 ************************/
#define SFC_WREN        0x06        //串行Flash命令集
#define SFC_WRDI        0x04
#define SFC_RDSR        0x05
#define SFC_WRSR        0x01
#define SFC_READ        0x03
#define SFC_FASTREAD    0x0B
#define SFC_RDID        0xAB
#define SFC_PAGEPROG    0x02
#define SFC_RDCR        0xA1
#define SFC_WRCR        0xF1
#define SFC_SECTORER1   0xD7        //PM25LV040 扇区擦除指令
#define SFC_SECTORER2   0x20        //W25Xxx 扇区擦除指令
#define SFC_BLOCKER     0xD8
#define SFC_CHIPER      0xC7

#define SPI_CE_High()   FLASH_CE  = 1     // set CE high
#define SPI_CE_Low()    FLASH_CE  = 0     // clear CE low
#define SPI_Hold()      P_SPI_Hold      = 0     // clear Hold pin
#define SPI_UnHold()    P_SPI_Hold      = 1     // set Hold pin
#define SPI_WP()        P_SPI_WP        = 0     // clear WP pin
#define SPI_UnWP()      P_SPI_WP        = 1     // set WP pin

/************************************************************************/
void SPI_Delay(u8 cnt)
{
    while(cnt--);
}

/************************************************************************/
void SPI_init(void)
{
    SPI_CE_High();
    FLASH_SCK = 0;    // set clock to low initial state
    FLASH_SI = 1;
}

/************************************************************************/
void SPI_WriteByte(u8 out)
{
    u8 i;
    i = 8;
    do{
        out <<= 1;
        FLASH_SI  = CY;
        FLASH_SCK = 1;
        SPI_Delay(3);
        FLASH_SCK = 0;
    }while(--i);
    FLASH_SI = 1;
}

/************************************************************************/
u8 SPI_ReadByte(void)
{
    u8 i, in;
    i = 8;
    do{
        in <<= 1;
        if (FLASH_SO) in++;
        FLASH_SCK = 1;
        SPI_Delay(3);
        FLASH_SCK = 0;
    }while(--i);
    return in;
}

/************************************************
检测Flash是否准备就绪
入口参数: 无
出口参数:
    0 : 没有检测到正确的Flash
    1 : Flash准备就绪
************************************************/
void FlashCheckID(void)
{
    SPI_CE_Low();
    SPI_WriteByte(SFC_RDID);        //发送读取ID命令
    SPI_WriteByte(0x00);            //空读3个字节
    SPI_WriteByte(0x00);
    SPI_WriteByte(0x00);
    FLASH_ID1 = SPI_ReadByte();         //读取制造商ID1
    FLASH_ID  = SPI_ReadByte();         //读取设备ID
    FLASH_ID2 = SPI_ReadByte();         //读取制造商ID2
    SPI_CE_High();

    if((FLASH_ID1 == 0x9d) && (FLASH_ID2 == 0x7f))  B_FlashOK = 1;  //检测是否为PM25LVxx系列的Flash
    else if(FLASH_ID == 0x12)  B_FlashOK = 2;                       //检测是否为W25X4x系列的Flash
    else if(FLASH_ID == 0x13)  B_FlashOK = 3;                       //检测是否为W25X8x系列的Flash
    else if(FLASH_ID == 0x16)  B_FlashOK = 4;                       //检测是否为W25X64系列的Flash
    else if(FLASH_ID == 0x17)  B_FlashOK = 5;                       //检测是否为W25X128系列的Flash
    else                                            B_FlashOK = 0;
}

/************************************************
检测Flash的忙状态
入口参数: 无
出口参数:
    0 : Flash处于空闲状态
    1 : Flash处于忙状态
************************************************/
u8  CheckFlashBusy(void)
{
    u8  dat;

    SPI_CE_Low();
    SPI_WriteByte(SFC_RDSR);        //发送读取状态命令
    dat = SPI_ReadByte();           //读取状态
    SPI_CE_High();

    return (dat);                   //状态值的Bit0即为忙标志
}

/************************************************
使能Flash写命令
入口参数: 无
出口参数: 无
************************************************/
void FlashWriteEnable(void)
{
    while(CheckFlashBusy() > 0);    //Flash忙检测
    SPI_CE_Low();
    SPI_WriteByte(SFC_WREN);        //发送写使能命令
    SPI_CE_High();
}

/************************************************
擦除整片Flash
入口参数: 无
出口参数: 无
************************************************/
/*
void FlashChipErase(void)
{
    if(B_FlashOK)
    {
        FlashWriteEnable();             //使能Flash写命令
        SPI_CE_Low();
        SPI_WriteByte(SFC_CHIPER);      //发送片擦除命令
        SPI_CE_High();
    }
}
*/

/************************************************
擦除扇区, 一个扇区4KB
入口参数: 无
出口参数: 无
************************************************/
void FlashSectorErase(u32 addr)
{
    if(B_FlashOK)
    {
        FlashWriteEnable();             //使能Flash写命令
        SPI_CE_Low();
        if(B_FlashOK == 1)
        {
            SPI_WriteByte(SFC_SECTORER1);    //发送扇区擦除命令
        }
        else
        {
            SPI_WriteByte(SFC_SECTORER2);    //发送扇区擦除命令
        }
        SPI_WriteByte(((u8 *)&addr)[1]);           //设置起始地址
        SPI_WriteByte(((u8 *)&addr)[2]);
        SPI_WriteByte(((u8 *)&addr)[3]);
        SPI_CE_High();
    }
}

/************************************************
从Flash中读取数据
入口参数:
    addr   : 地址参数
    buffer : 缓冲从Flash中读取的数据
    size   : 数据块大小
出口参数:
    无
************************************************/
void SPI_Read_Nbytes(u32 addr, u8 *buffer, u16 size)
{
    if(size == 0)   return;
    if(!B_FlashOK)  return;
    while(CheckFlashBusy() > 0);        //Flash忙检测

    SPI_CE_Low();                       //enable device
    SPI_WriteByte(SFC_READ);            //read command

    SPI_WriteByte(((u8 *)&addr)[1]);    //设置起始地址
    SPI_WriteByte(((u8 *)&addr)[2]);
    SPI_WriteByte(((u8 *)&addr)[3]);

    do{
        *buffer = SPI_ReadByte();       //receive byte and store at buffer
        buffer++;
    }while(--size);                     //read until no_bytes is reached
    SPI_CE_High();                      //disable device
}

/************************************************************************
读出n个字节,跟指定的数据进行比较, 错误返回1,正确返回0
************************************************************************/
u8  SPI_Read_Compare(u32 addr, u8 *buffer, u16 size)
{
    u8  j;
    if(size == 0)   return 2;
    if(!B_FlashOK)  return 2;
    while(CheckFlashBusy() > 0);            //Flash忙检测

    j = 0;
    SPI_CE_Low();                           //enable device
    SPI_WriteByte(SFC_READ);                //read command
    SPI_WriteByte(((u8 *)&addr)[1]);        //设置起始地址
    SPI_WriteByte(((u8 *)&addr)[2]);
    SPI_WriteByte(((u8 *)&addr)[3]);
    do
    {
        if(*buffer != SPI_ReadByte())       //receive byte and store at buffer
        {
            j = 1;
            break;
        }
        buffer++;
    }while(--size);         //read until no_bytes is reached
    SPI_CE_High();          //disable device
    return j;
}

/************************************************
写数据到Flash中
入口参数:
    addr   : 地址参数
    buffer : 缓冲需要写入Flash的数据
    size   : 数据块大小
出口参数: 无
************************************************/
void SPI_Write_Nbytes(u32 addr, u8 *buffer, u8 size)
{
    if(size == 0)   return;
    if(!B_FlashOK)  return;
    while(CheckFlashBusy() > 0);        //Flash忙检测

    FlashWriteEnable();                 //使能Flash写命令

    SPI_CE_Low();                       // enable device
    SPI_WriteByte(SFC_PAGEPROG);        // 发送页编程命令
    SPI_WriteByte(((u8 *)&addr)[1]);    //设置起始地址
    SPI_WriteByte(((u8 *)&addr)[2]);
    SPI_WriteByte(((u8 *)&addr)[3]);
    do{
        SPI_WriteByte(*buffer++);       //连续页内写
        addr++;
        if ((addr & 0xff) == 0) break;
    }while(--size);
    SPI_CE_High();                      // disable device
}

/******************** 串口打印函数 ********************/
void UartInit(void)
{
    S1_S1 = 0;      //UART1 switch to, 00: P3.0 P3.1, 01: P3.6 P3.7, 10: P1.6 P1.7, 11: P4.3 P4.4
    S1_S0 = 0;
    SCON = (SCON & 0x3f) | 0x40; 
    T1x12 = 1;      //定时器时钟1T模式
    S1BRT = 0;      //串口1选择定时器1为波特率发生器
    TL1  = TM;
    TH1  = TM>>8;
    TR1 = 1;        //定时器1开始计时
    REN = 1;        //允许接收
    ES  = 1;        //允许中断

    UR1TOCR = 0xc0; //使能超时接收，使能超时中断，超时时钟选择 1:系统时钟  0:串口数据位率(波特率)
    UR1TOTL = 0x10;
    UR1TOTH = 0x00;
    UR1TOTE = 0x00; //需要写 UR1TOTE 后，新的TM值才会生效
}

void UartPutc(unsigned char dat)
{
    B_TX1_Busy = 1;
    SBUF = dat; 
    while(B_TX1_Busy);
}

char putchar(char c)
{
    UartPutc(c);
    return c;
}

//========================================================================
// 函数: void UART1_int (void) interrupt UART1_VECTOR
// 描述: UART1中断函数。
// 参数: nine.
// 返回: none.
// 版本: VER1.0
// 日期: 2024-11-28
// 备注: 
//========================================================================
void UART1_int (void) interrupt 4
{
    if(RI)
    {
        RI = 0;
        RX1_Buffer[RX1_Cnt] = SBUF;
        if(++RX1_Cnt >= UART_BUF_LENGTH)   RX1_Cnt = 0;
    }

    if(TI)
    {
        TI = 0;
        B_TX1_Busy = 0;
    }

    if(UR1TOSR & 0x01)
    {
        B_RX1_TimeOut = 1;
        UR1TOSR = 0x80;     //设置 RTOCF 清除超时标志位 TOIF
    }
}
