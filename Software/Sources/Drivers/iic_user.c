#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
//<<AICUBE_USER_GLOBAL_DEFINE_END>>

#define I2C1_DMA_BUF_SIZE 16
/* DMA 缓冲区 */
uint8_t xdata I2C1_DmaTxBuffer[I2C1_DMA_BUF_SIZE];
uint8_t xdata I2C1_DmaRxBuffer[I2C1_DMA_BUF_SIZE];

/* DMA 完成标志 */
bit I2C1_DmaTxFlag = 0;
bit I2C1_DmaRxFlag = 0;

/* ---- I2CMSCR 命令码 ---- */
#define MSCMD_START         0x01    /* 仅发送 START */
#define MSCMD_STOP          0x06    /* 仅发送 STOP */
#define MSCMD_START_SEND    0x09    /* START + 发送 I2CTXD + 接收 ACK */
#define MSCMD_SEND          0x0a    /* 发送 I2CTXD + 接收 ACK */
#define MSCMD_RECV_ACK      0x0b    /* 接收 1 字节 + 回应 ACK */
#define MSCMD_RECV_NAK      0x0c    /* 接收 1 字节 + 回应 NAK */
#define MSCMD_WRITE_COMBO   0x89    /* 主机写组合命令 (DMA) */
#define MSCMD_READ_COMBO    0x8b    /* 主机读组合命令 (DMA) */

/* ---- 寄存器位 ---- */
#define MSBUSY              0x80    /* I2CMSST bit7: 主机忙碌 */
#define MSIF                0x40    /* I2CMSST bit6: 主机中断标志 */
#define MSACKI              0x02    /* I2CMSST bit1: 接收到的 ACK 值 */
#define MSACKO              0x01    /* I2CMSST bit0: 要发送的 ACK 值 */


////////////////////////////////////////
// I2C1 初始化 (P7.6=SCL, P7.7=SDA, 轮询模式)
////////////////////////////////////////
void I2C1_Init(void)
{
    I2C1_SwitchP7677();             /* SCL=P7.6, SDA=P7.7 */
    I2CCFG = 0xe0;                  /* ENI2C=1, MSSL=1 (主机模式), SPEED=0 */
    I2C1_SetClockDivider(1600);     /* 设置时钟分频 */
    I2CMSST = 0x00;                 /* 清除所有状态 */
    DMA_I2C_CR = 0x00;              /* 关闭 DMA, 默认纯轮询模式 */
}


/* ==================== 轮询模式基本操作 ==================== */

////////////////////////////////////////
// 等待 I2C 主机命令完成 (轮询硬件 MSIF)
////////////////////////////////////////
void I2C1_Wait(void)
{
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;
}

////////////////////////////////////////
// 等待 I2C 主机空闲 (BUSY=0)
////////////////////////////////////////
void I2C1_WaitBusy(void)
{
    while (I2CMSST & MSBUSY);
}

////////////////////////////////////////
// 发送 START (独立的 START, 会先等空闲)
////////////////////////////////////////
void I2C1_StartCmd(void)
{
    I2C1_WaitBusy();
    I2CMSCR = MSCMD_START;
    I2C1_Wait();
}

////////////////////////////////////////
// 发送 STOP (帧结束, 不等空闲 — 此时主机正占着总线)
////////////////////////////////////////
void I2C1_StopCmd(void)
{
    I2CMSCR = MSCMD_STOP;
    I2C1_Wait();
}

////////////////////////////////////////
// START + 发送 1 字节 + 收 ACK (帧内部, 不等空闲)
// 返回: 0=ACK, 1=NAK
////////////////////////////////////////
static uint8_t I2C1_SendByteStart(uint8_t dat)
{
    I2CTXD = dat;
    I2CMSCR = MSCMD_START_SEND;
    I2C1_Wait();
    return (I2CMSST & MSACKI) ? 1 : 0;
}

////////////////////////////////////////
// 发送 1 字节 + 收 ACK (帧内部, 不等空闲)
// 返回: 0=ACK, 1=NAK
////////////////////////////////////////
static uint8_t I2C1_SendByte(uint8_t dat)
{
    I2CTXD = dat;
    I2CMSCR = MSCMD_SEND;
    I2C1_Wait();
    return (I2CMSST & MSACKI) ? 1 : 0;
}

////////////////////////////////////////
// 接收 1 字节 + 回应 ACK (帧内部, 不等空闲)
////////////////////////////////////////
static uint8_t I2C1_RecvByteAck(void)
{
    uint8_t dat;
    I2CMSCR = MSCMD_RECV_ACK;
    I2C1_Wait();
    dat = I2CRXD;
    return dat;
}

////////////////////////////////////////
// 接收 1 字节 + 回应 NAK (帧内部, 不等空闲)
////////////////////////////////////////
static uint8_t I2C1_RecvByteNak(void)
{
    uint8_t dat;
    I2CMSCR = MSCMD_RECV_NAK;
    I2C1_Wait();
    dat = I2CRXD;
    return dat;
}


/* ==================== 设备寄存器读写 (16-bit 寄存器地址) ==================== */

////////////////////////////////////////
// 读设备寄存器 1 字节
// dev_wr_addr: 设备 7-bit 地址左移1位 (写地址)
// reg_addr:    16-bit 寄存器地址
////////////////////////////////////////
uint8_t I2C1_ReadRegByte(uint8_t dev_wr_addr, uint16_t reg_addr)
{
    uint8_t dev_rd_addr;
    uint8_t dat;

    dev_rd_addr = dev_wr_addr | 0x01;

    I2C1_WaitBusy();   /* 等上一次事务结束 */

    /* 第1帧: START + 写地址 + 寄存器地址高字节 + 寄存器地址低字节 */
    I2C1_SendByteStart(dev_wr_addr);
    I2C1_SendByte((uint8_t)(reg_addr >> 8));
    I2C1_SendByte((uint8_t)(reg_addr & 0xFF));

    /* 第2帧: 重复 START + 读地址 */
    I2C1_SendByteStart(dev_rd_addr);

    /* 接收 1 字节 + NAK */
    dat = I2C1_RecvByteNak();

    /* STOP */
    I2C1_StopCmd();

    return dat;
}

////////////////////////////////////////
// 读设备寄存器 2 字节 (大端序)
////////////////////////////////////////
uint16_t I2C1_ReadRegWord(uint8_t dev_wr_addr, uint16_t reg_addr)
{
    uint8_t dev_rd_addr;
    uint8_t hi;
    uint8_t lo;

    dev_rd_addr = dev_wr_addr | 0x01;

    I2C1_WaitBusy();   /* 等上一次事务结束 */

    /* 第1帧: START + 写地址 + 寄存器地址高字节 + 寄存器地址低字节 */
    I2C1_SendByteStart(dev_wr_addr);
    I2C1_SendByte((uint8_t)(reg_addr >> 8));
    I2C1_SendByte((uint8_t)(reg_addr & 0xFF));

    /* 第2帧: 重复 START + 读地址 */
    I2C1_SendByteStart(dev_rd_addr);

    /* 接收 2 字节: 第1字节 ACK, 第2字节 NAK */
    hi = I2C1_RecvByteAck();
    lo = I2C1_RecvByteNak();

    /* STOP */
    I2C1_StopCmd();

    return ((uint16_t)hi << 8) | (uint16_t)lo;
}

////////////////////////////////////////
// 写设备寄存器 1 字节
////////////////////////////////////////
void I2C1_WriteRegByte(uint8_t dev_wr_addr, uint16_t reg_addr, uint8_t dat)
{
    I2C1_WaitBusy();   /* 等上一次事务结束 */

    /* START + 写地址 + 寄存器地址高 + 寄存器地址低 + 数据 */
    I2C1_SendByteStart(dev_wr_addr);
    I2C1_SendByte((uint8_t)(reg_addr >> 8));
    I2C1_SendByte((uint8_t)(reg_addr & 0xFF));
    I2C1_SendByte(dat);

    /* STOP */
    I2C1_StopCmd();
}


/* ==================== 轮询模式多字节读写 (VL53L1X 等) ==================== */

////////////////////////////////////////
// 写设备寄存器多字节 (16-bit 寄存器地址)
// Protocol: START + dev_wr_addr(W) + reg_h + reg_l + data[0..count-1] + STOP
////////////////////////////////////////
void I2C1_WriteRegMulti(uint8_t dev_wr_addr, uint16_t reg_addr,
                        uint8_t *pdat, uint8_t count)
{
    uint8_t i;

    I2C1_WaitBusy();

    I2C1_SendByteStart(dev_wr_addr);
    I2C1_SendByte((uint8_t)(reg_addr >> 8));
    I2C1_SendByte((uint8_t)(reg_addr & 0xFF));

    for (i = 0; i < count; i++) {
        I2C1_SendByte(pdat[i]);
    }

    I2C1_StopCmd();
}

////////////////////////////////////////
// 读设备寄存器多字节 (16-bit 寄存器地址)
// Protocol: START + dev_wr_addr(W) + reg_h + reg_l
//         + RESTART + dev_rd_addr(R)
//         + read[0..count-2](ACK) + read[count-1](NAK) + STOP
////////////////////////////////////////
void I2C1_ReadRegMulti(uint8_t dev_wr_addr, uint16_t reg_addr,
                       uint8_t *pdat, uint8_t count)
{
    uint8_t i;
    uint8_t dev_rd_addr;

    dev_rd_addr = dev_wr_addr | 0x01;

    I2C1_WaitBusy();

    /* 第1帧: START + 写地址 + 寄存器地址高字节 + 寄存器地址低字节 */
    I2C1_SendByteStart(dev_wr_addr);
    I2C1_SendByte((uint8_t)(reg_addr >> 8));
    I2C1_SendByte((uint8_t)(reg_addr & 0xFF));

    /* 第2帧: 重复 START + 读地址 */
    I2C1_SendByteStart(dev_rd_addr);

    /* 接收 count 字节: 前 count-1 字节 ACK, 最后 1 字节 NAK */
    for (i = 0; i < count - 1; i++) {
        pdat[i] = I2C1_RecvByteAck();
    }
    pdat[count - 1] = I2C1_RecvByteNak();

    /* STOP */
    I2C1_StopCmd();
}

/* ==================== DMA 模式 (参照官方例程) ==================== */

////////////////////////////////////////
// DMA 硬件配置 (不触发传输)
////////////////////////////////////////
void I2C1_DMA_Config(void)
{
    /* 发送 DMA */
    DMA_I2CT_STA = 0x00;
    DMA_I2CT_CFG = 0x80;            /* 使能中断 */
    DMA_I2CT_AMT = 0xff;            /* 传输总字节数低8位 */
    DMA_I2CT_AMTH = 0x00;           /* 传输总字节数高8位 */
    DMA_I2CT_TXAH = (uint8_t)((uint16_t)&I2C1_DmaTxBuffer >> 8);
    DMA_I2CT_TXAL = (uint8_t)((uint16_t)&I2C1_DmaTxBuffer);
    DMA_I2CT_CR = 0x80;             /* 使能 DMA TX, 不触发 */

    /* 接收 DMA */
    DMA_I2CR_STA = 0x00;
    DMA_I2CR_CFG = 0x80;            /* 使能中断 */
    DMA_I2CR_AMT = 0xff;            /* 传输总字节数低8位 */
    DMA_I2CR_AMTH = 0x00;           /* 传输总字节数高8位 */
    DMA_I2CR_RXAH = (uint8_t)((uint16_t)&I2C1_DmaRxBuffer >> 8);
    DMA_I2CR_RXAL = (uint8_t)((uint16_t)&I2C1_DmaRxBuffer);
    DMA_I2CR_CR = 0x81;             /* 使能 DMA RX, 清除 FIFO */

    /* 总传输字节数 */
    DMA_I2C_ST1 = 0xff;
    DMA_I2C_ST2 = 0x00;
}

////////////////////////////////////////
// DMA 写入 N 字节 (1 字节存储地址, AT24C02 等)
// 调用前: I2C1_DmaTxBuffer[2..2+len-1] 填入数据
////////////////////////////////////////
void I2C1_DMA_WriteNbyte(uint8_t dev_wr_addr, uint8_t mem_addr, uint8_t len)
{
    while (I2CMSST & MSBUSY);       /* 等待空闲 */

    I2C1_DmaTxFlag = 1;
    I2C1_DmaTxBuffer[0] = dev_wr_addr;
    I2C1_DmaTxBuffer[1] = mem_addr;

    I2CMSST = 0x00;
    I2CMSCR = MSCMD_WRITE_COMBO;    /* START + 发送 + 自动 ACK (DMA 控制) */
    DMA_I2C_CR = 0x01;              /* DMAEN=0 (先关) */

    DMA_I2CT_AMT = len + 1;         /* 总字节: 数据 + 设备地址 + 存储地址 */
    DMA_I2CT_AMTH = 0x00;
    DMA_I2C_ST1 = len + 1;
    DMA_I2C_ST2 = 0x00;
    DMA_I2CT_CR |= 0x40;            /* TRIG=1, 启动 DMA 发送 */

    while (I2C1_DmaTxFlag);         /* 等待完成 */
    DMA_I2C_CR = 0x00;
}

////////////////////////////////////////
// DMA 读出 N 字节 (1 字节存储地址)
// 数据读到 I2C1_DmaRxBuffer[0..len-1]
////////////////////////////////////////
void I2C1_DMA_ReadNbyte(uint8_t dev_wr_addr, uint8_t dev_rd_addr,
                        uint8_t mem_addr, uint8_t len)
{
    while (I2CMSST & MSBUSY);
    DMA_I2C_CR = 0x00;
    I2CMSST = 0x00;

    /* 手动发送: START + 设备写地址 */
    I2CTXD = dev_wr_addr;
    I2CMSCR = MSCMD_START_SEND;
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;

    /* 发送: 存储地址 */
    I2CTXD = mem_addr;
    I2CMSCR = MSCMD_SEND;
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;

    /* 发送: 重复 START + 设备读地址 */
    I2CTXD = dev_rd_addr;
    I2CMSCR = MSCMD_START_SEND;
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;

    /* 启动 DMA 接收 */
    I2C1_DmaRxFlag = 1;
    I2CMSCR = MSCMD_READ_COMBO;
    DMA_I2C_CR = 0x01;

    DMA_I2CR_AMT = len - 1;
    DMA_I2CR_AMTH = 0x00;
    DMA_I2C_ST1 = len - 1;
    DMA_I2C_ST2 = 0x00;
    DMA_I2CR_CR |= 0x40;            /* TRIG */

    while (I2C1_DmaRxFlag);
    DMA_I2C_CR = 0x00;
}

////////////////////////////////////////
// DMA 寄存器写入 (16-bit 寄存器地址, VL6180X 等)
// 调用前: I2C1_DmaTxBuffer[3..3+len-1] 填入数据
////////////////////////////////////////
void I2C1_DMA_WriteReg(uint8_t dev_wr_addr, uint16_t reg_addr, uint8_t len)
{
    while (I2CMSST & MSBUSY);

    I2C1_DmaTxFlag = 1;
    I2C1_DmaTxBuffer[0] = dev_wr_addr;
    I2C1_DmaTxBuffer[1] = (uint8_t)(reg_addr >> 8);    /* 寄存器地址高字节 */
    I2C1_DmaTxBuffer[2] = (uint8_t)(reg_addr & 0xFF);  /* 寄存器地址低字节 */

    I2CMSST = 0x00;
    I2CMSCR = MSCMD_WRITE_COMBO;
    DMA_I2C_CR = 0x01;

    DMA_I2CT_AMT = len + 2;         /* 总字节: 数据 + 设备地址 + 2字节寄存器地址 */
    DMA_I2CT_AMTH = 0x00;
    DMA_I2C_ST1 = len + 2;
    DMA_I2C_ST2 = 0x00;
    DMA_I2CT_CR |= 0x40;

    while (I2C1_DmaTxFlag);
    DMA_I2C_CR = 0x00;
}

////////////////////////////////////////
// DMA 寄存器读出 (16-bit 寄存器地址)
// 数据读到 I2C1_DmaRxBuffer[0..len-1]
////////////////////////////////////////
void I2C1_DMA_ReadReg(uint8_t dev_wr_addr, uint8_t dev_rd_addr,
                      uint16_t reg_addr, uint8_t len)
{
    while (I2CMSST & MSBUSY);
    DMA_I2C_CR = 0x00;
    I2CMSST = 0x00;

    /* 手动: START + 设备写地址 */
    I2CTXD = dev_wr_addr;
    I2CMSCR = MSCMD_START_SEND;
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;

    /* 手动: 寄存器地址高字节 */
    I2CTXD = (uint8_t)(reg_addr >> 8);
    I2CMSCR = MSCMD_SEND;
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;

    /* 手动: 寄存器地址低字节 */
    I2CTXD = (uint8_t)(reg_addr & 0xFF);
    I2CMSCR = MSCMD_SEND;
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;

    /* 手动: 重复 START + 设备读地址 */
    I2CTXD = dev_rd_addr;
    I2CMSCR = MSCMD_START_SEND;
    while ((I2CMSST & MSIF) == 0);
    I2CMSST &= ~MSIF;

    /* 启动 DMA 接收 */
    I2C1_DmaRxFlag = 1;
    I2CMSCR = MSCMD_READ_COMBO;
    DMA_I2C_CR = 0x01;

    DMA_I2CR_AMT = len - 1;
    DMA_I2CR_AMTH = 0x00;
    DMA_I2C_ST1 = len - 1;
    DMA_I2C_ST2 = 0x00;
    DMA_I2CR_CR |= 0x40;

    while (I2C1_DmaRxFlag);
    DMA_I2C_CR = 0x00;
}


/* ==================== 中断服务程序 ==================== */

////////////////////////////////////////
// I2C1 主机中断
////////////////////////////////////////
void I2C1_ISR(void) interrupt I2C1_VECTOR
{
    //<<AICUBE_USER_I2C1_ISR_CODE1_BEGIN>>
    if (I2C1_CheckMasterFlag())
    {
        I2C1_ClearMasterFlag();
    }
    //<<AICUBE_USER_I2C1_ISR_CODE1_END>>
}

////////////////////////////////////////
// I2C1 DMA 发送完成中断
////////////////////////////////////////
void DMA_I2C1TX_ISR(void) interrupt DMA_I2C1T_VECTOR
{
    //<<AICUBE_USER_I2C1_ISR_CODE2_BEGIN>>
    if (DMA_I2C1_CheckTxFlag())
    {
        DMA_I2C1_ClearTxFlag();
        I2C1_DmaTxFlag = 0;
    }
    if (DMA_I2C1_CheckOverWriteFlag())
    {
        DMA_I2C1_ClearOverWriteFlag();
    }
    //<<AICUBE_USER_I2C1_ISR_CODE2_END>>
}

////////////////////////////////////////
// I2C1 DMA 接收完成中断
////////////////////////////////////////
void DMA_I2C1RX_ISR(void) interrupt DMA_I2C1R_VECTOR
{
    if (DMA_I2C1_CheckRxFlag())
    {
        DMA_I2C1_ClearRxFlag();
        I2C1_DmaRxFlag = 0;
    }
    if (DMA_I2C1_CheckRxLossFlag())
    {
        DMA_I2C1_ClearRxLossFlag();
    }
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>
