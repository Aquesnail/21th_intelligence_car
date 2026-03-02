//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: i2c.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-03-02) 创建文件
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
// I2C2初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C2_Init(void)
{
    I2C2_SwitchP2627();                 //选择I2C2数据口: SCL(P2.7), SDA(P2.6)

    I2C2_MasterMode();                  //设置I2C2为主机模式
    I2C2_SetClockDivider(1000);          //设置I2C2为主机模式时钟

    I2C2_Enable();                      //使能I2C2功能

    //<<AICUBE_USER_I2C2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_I2C2_INITIAL_END>>
}

////////////////////////////////////////
// I2C2主机模式等待命令完成
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C2_MasterWait(void)
{
    while (!I2C2_CheckMasterFlag());    //等待完成标志
    I2C2_ClearMasterFlag();             //清除完成标志
    I2C2_Idle();                        //恢复IDLE状态
}

////////////////////////////////////////
// I2C2主机模式发送起始信号
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C2_MasterStart(void)
{
    I2C2_Start();                       //触发主机模式起始命令
    I2C2_MasterWait();                  //等待命令完成
}

////////////////////////////////////////
// I2C2主机模式发送停止信号
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C2_MasterStop(void)
{
    I2C2_Stop();                        //触发主机模式停止命令
    I2C2_MasterWait();                  //等待命令完成
}

////////////////////////////////////////
// I2C2主机模式发送1字节数据
// 入口参数: dat (待发送的字节数据)
// 函数返回: 0   (接收的应答信号为ACK)
//           1   (接收的应答信号为NAK)
////////////////////////////////////////
uint8_t I2C2_MasterSendByte(uint8_t dat)
{
    I2C2_WriteData(dat);                //将数据写入I2C2数据寄存器
    I2C2_SendData();                    //触发主机模式写数据命令
    I2C2_MasterWait();                  //等待命令完成
    I2C2_RecvACK();                     //触发主机模式接收应答命令
    I2C2_MasterWait();                  //等待命令完成

    return I2C2_MasterReadACK();        //读取并返回应答信号
}

////////////////////////////////////////
// I2C2主机模式接收1字节数据
// 入口参数: ack (待发送的应答信号)
// 函数返回:     (接收的字节数据)
////////////////////////////////////////
uint8_t I2C2_MasterReadByte(uint8_t ack)
{
    uint8_t dat;

    I2C2_RecvData();                    //触发主机模式读数据命令
    I2C2_MasterWait();                  //等待命令完成
    dat = I2C2_ReadData();              //读取接收的数据
    if (!ack)
        I2C2_MasterSetACK();            //将ACK数据写入寄存器
    else
        I2C2_MasterSetNAK();            //将NAK数据写入寄存器
    I2C2_SendACK();                     //触发主机模式发送应答命令
    I2C2_MasterWait();                  //等待命令完成

    return dat;                         //返回接收的数据
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


