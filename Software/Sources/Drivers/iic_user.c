#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


BOOL fI2C1MasterBusy;                   //I2C1主模式忙标志
uint8_t xdata pu8I2C1DMATxBuffer[16];   //I2C1 DMA发送缓冲区数组
uint8_t xdata pu8I2C1DMARxBuffer[16];   //I2C1 DMA接收缓冲区数组

////////////////////////////////////////
// I2C1初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C1_Init(void)
{
    I2C1_SwitchP7677();                 //选择I2C1数据口: SCL(P3.2), SDA(P3.3)

    I2C1_MasterMode();                  //设置I2C1为主机模式
    I2C1_SetClockDivider(200);          //设置I2C1为主机模式时钟

    I2C1_SetIntPriority(0);             //设置中断为最低优先级
    I2C1_EnableMasterInt();             //设置I2C1主机模式中断
    fI2C1MasterBusy = 0;                //清除主机模式忙标志位
    DMA_I2C1_EnableACKErrorInt();       //使能I2C1接收ACK错误中断

    I2C1_Enable();                      //使能I2C1功能

    DMA_I2C1_EnableDMA();               //使能I2C1 DMA
    DMA_I2C1_SetDMAAmount(15);          //使能I2C1 DMA发送/接收总字节数
    DMA_I2C1_SetTxAmount(15);           //设置I2C1 DMA发送总字节数
    DMA_I2C1_SetTxAddress(pu8I2C1DMATxBuffer); //设置I2C1 DMA发送缓冲区地址
    DMA_I2C1_SetInterval(0);            //设置I2C1 DMA发送/接收字节间隔时间（系统时钟）
    DMA_I2C1_ClearTxFlag();             //清除I2C1 发送DMA中断标志
    DMA_I2C1_SetTxBusPriority(0);       //设置总线访问为最低优先级
    DMA_I2C1_SetTxIntPriority(0);       //设置中断为最低优先级
    DMA_I2C1_EnableTxInt();             //使能I2C1 DMA发送中断
    DMA_I2C1_EnableTx();                //使能I2C1 DMA发送功能
//  DMA_I2C1_TriggerTx();               //触发I2C1 DMA发送

    DMA_I2C1_EnableDMA();               //使能I2C1 DMA
    DMA_I2C1_SetDMAAmount(15);          //使能I2C1 DMA发送/接收总字节数
    DMA_I2C1_SetRxAmount(15);           //设置I2C1 DMA接收总字节数
    DMA_I2C1_SetRxAddress(pu8I2C1DMARxBuffer); //设置I2C1 DMA接收缓冲区地址
    DMA_I2C1_SetInterval(0);            //设置I2C1 DMA发送/接收字节间隔时间（系统时钟）
    DMA_I2C1_ClearFIFO();               //清空I2C1 DMA接收FIFO缓冲区
    DMA_I2C1_ClearRxFlag();             //清除I2C1 接收DMA中断标志
    DMA_I2C1_SetRxBusPriority(0);       //设置总线访问为最低优先级
    DMA_I2C1_EnableRx();                //使能I2C1 DMA接收功能
//  DMA_I2C1_TriggerRx();               //触发I2C1 DMA接收

    //<<AICUBE_USER_I2C1_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_I2C1_INITIAL_END>>
}

////////////////////////////////////////
// I2C1主机模式等待命令完成
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C1_MasterWait(void)
{
    while (fI2C1MasterBusy);            //等待忙标志为0
}

////////////////////////////////////////
// I2C1主机模式发送起始信号
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C1_MasterStart(void)
{
    I2C1_Start();                       //触发主机模式起始命令
    fI2C1MasterBusy = 1;                //设置忙标志
    I2C1_MasterWait();                  //等待命令完成
}

////////////////////////////////////////
// I2C1主机模式发送停止信号
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C1_MasterStop(void)
{
    I2C1_Stop();                        //触发主机模式停止命令
    fI2C1MasterBusy = 1;                //设置忙标志
    I2C1_MasterWait();                  //等待命令完成
}

////////////////////////////////////////
// I2C1主机模式发送1字节数据
// 入口参数: dat (待发送的字节数据)
// 函数返回: 0   (接收的应答信号为ACK)
//           1   (接收的应答信号为NAK)
////////////////////////////////////////
uint8_t I2C1_MasterSendByte(uint8_t dat)
{
    I2C1_WriteData(dat);                //将数据写入I2C1数据寄存器
    I2C1_SendData();                    //触发主机模式写数据命令
    fI2C1MasterBusy = 1;                //设置忙标志
    I2C1_MasterWait();                  //等待命令完成
    I2C1_RecvACK();                     //触发主机模式接收应答命令
    fI2C1MasterBusy = 1;                //设置忙标志
    I2C1_MasterWait();                  //等待命令完成

    return I2C1_MasterReadACK();        //读取并返回应答信号
}

////////////////////////////////////////
// I2C1主机模式接收1字节数据
// 入口参数: ack (待发送的应答信号)
// 函数返回:     (接收的字节数据)
////////////////////////////////////////
uint8_t I2C1_MasterReadByte(uint8_t ack)
{
    uint8_t dat;

    I2C1_RecvData();                    //触发主机模式读数据命令
    fI2C1MasterBusy = 1;                //设置忙标志
    I2C1_MasterWait();                  //等待命令完成
    dat = I2C1_ReadData();              //读取接收的数据
    if (!ack)
        I2C1_MasterSetACK();            //将ACK数据写入寄存器
    else
        I2C1_MasterSetNAK();            //将NAK数据写入寄存器
    I2C1_SendACK();                     //触发主机模式发送应答命令
    fI2C1MasterBusy = 1;                //设置忙标志
    I2C1_MasterWait();                  //等待命令完成

    return dat;                         //返回接收的数据
}


////////////////////////////////////////
// I2C1中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C1_ISR(void) interrupt I2C1_VECTOR
{
    //<<AICUBE_USER_I2C1_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    if (I2C1_CheckMasterFlag())         //判断I2C1主机中断
    {
        I2C1_ClearMasterFlag();         //清除I2C1主机中断标志
        fI2C1MasterBusy = 0;            //清除主机模式忙标志位
    }
    //<<AICUBE_USER_I2C1_ISR_CODE1_END>>
}

////////////////////////////////////////
// I2C1发送DMA中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void DMA_I2C1TX_ISR(void) interrupt DMA_I2C1T_VECTOR
{
    //<<AICUBE_USER_I2C1_ISR_CODE2_BEGIN>>
    // 在此添加中断函数用户代码  
    if (DMA_I2C1_CheckTxFlag())         //判断I2C1发送DMA中断
    {
        DMA_I2C1_ClearTxFlag();         //清除I2C1发送DMA中断标志
    }
    //<<AICUBE_USER_I2C1_ISR_CODE2_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


