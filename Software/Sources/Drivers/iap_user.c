////////////////////////////////////////
// 文件名称: iap_user.c
// 文件描述: EEPROM 底层驱动（参照官方 EEPROM.c 实现）
// 文件版本: V3.0
// 修改记录:
//   1. (2026-01-17) 创建文件
//   2. (2026-04-06) 修复：使用相对地址、添加 WTST 配置、中断保护、自检
// 重要说明：
//   - 本文件使用单字节 IAP 操作方式，与官方 EEPROM.c 保持一致
//   - 虽然函数名包含 "IAP_"，但内部实现为 EEPROM 操作方式
//   - EEPROM 地址范围：0x0000 ~ 0x37FF（相对于 FC:2800h）
////////////////////////////////////////

#include "config.h"

// 根据主频配置 Flash 等待周期（关键！）
#if (MAIN_Fosc >= 120000000L)
    #define IAP_WTST_VALUE  3
#elif (MAIN_Fosc >= 90000000L)
    #define IAP_WTST_VALUE  2
#elif (MAIN_Fosc >= 60000000L)
    #define IAP_WTST_VALUE  1
#else
    #define IAP_WTST_VALUE  0
#endif

// EEPROM 区域定义（相对地址）
// STC32G144K246 EEPROM 位于 Flash FC:2800h~FF:FFFFh
// IAP 操作时使用相对地址 0x0000~0x37FF
#define EEPROM_START_ADDR       0x0000
#define EEPROM_SIZE             (14 * 512)  // 14个扇区，每个512字节

// 自检测试地址（使用最后一个扇区的末尾，避免与用户数据冲突）
#define IAP_TEST_ADDR           (EEPROM_SIZE - 32)  // 距离末尾32字节

// IAP 命令定义
#define IAP_STANDBY()           IAP_CMD = 0
#define IAP_READ()              IAP_CMD = 1
#define IAP_WRITE()             IAP_CMD = 2
#define IAP_ERASE()             IAP_CMD = 3

// 带中断保护的 IAP 触发（关键！防止触发序列被中断打断）
// 注意：命名为 _IAP_TriggerCmd 以避免与头文件中的 IAP_Trigger 宏冲突
static void _IAP_TriggerCmd(void)
{
    uint8_t ea_save;

    ea_save = EA;       // 保存全局中断状态
    EA = 0;             // 禁止中断

    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;    // 写入触发序列

    // STC32G 需要 4 个 NOP 保证数据稳定
    _nop_();
    _nop_();
    _nop_();
    _nop_();

    EA = ea_save;       // 恢复全局中断
}

// 禁止 EEPROM 操作（关闭 IAP 模块）
static void DisableEEPROM(void)
{
    IAP_CONTR = 0;
    IAP_CMD = 0;
    IAP_TRIG = 0;
    IAP_ADDRE = 0xFF;
    IAP_ADDRH = 0xFF;
    IAP_ADDRL = 0xFF;
}

// EEPROM 扇区擦除
// 注意：addr 是相对地址（0x0000 起始）
static void EEPROM_SectorErase(uint32_t addr)
{
    // 设置 IAP 时基并启用 IAP
    IAP_CONTR = 0x80;                   // IAPEN = 1
    IAP_TPS = MAIN_Fosc / 1000000;

    IAP_ERASE();                        // 设置擦除命令

    // 设置地址（24位）
    IAP_ADDRE = (uint8_t)(addr >> 16);
    IAP_ADDRH = (uint8_t)(addr >> 8);
    IAP_ADDRL = (uint8_t)addr;

    _IAP_TriggerCmd();                  // 触发命令（带中断保护）
    DisableEEPROM();                    // 关闭 IAP
}

// EEPROM 字节写入
// 注意：addr 是相对地址（0x0000 起始）
static void EEPROM_WriteByte(uint32_t addr, uint8_t dat)
{
    // 设置 IAP 时基并启用 IAP
    IAP_CONTR = 0x80;                   // IAPEN = 1
    IAP_TPS = MAIN_Fosc / 1000000;

    IAP_WRITE();                        // 设置字节编程命令

    // 设置地址
    IAP_ADDRE = (uint8_t)(addr >> 16);
    IAP_ADDRH = (uint8_t)(addr >> 8);
    IAP_ADDRL = (uint8_t)addr;

    // 写入数据
    IAP_DATA = dat;

    _IAP_TriggerCmd();                  // 触发命令（带中断保护）
    DisableEEPROM();                    // 关闭 IAP
}

// EEPROM 字节读取
// 注意：addr 是相对地址（0x0000 起始）
static uint8_t EEPROM_ReadByte(uint32_t addr)
{
    uint8_t dat;

    // 设置 IAP 时基并启用 IAP
    IAP_CONTR = 0x80;                   // IAPEN = 1
    IAP_TPS = MAIN_Fosc / 1000000;

    IAP_READ();                         // 设置读取命令

    // 设置地址
    IAP_ADDRE = (uint8_t)(addr >> 16);
    IAP_ADDRH = (uint8_t)(addr >> 8);
    IAP_ADDRL = (uint8_t)addr;

    _IAP_TriggerCmd();                  // 触发命令（带中断保护）

    dat = IAP_DATA;                     // 读取数据
    DisableEEPROM();                    // 关闭 IAP

    return dat;
}

////////////////////////////////////////
// IAP初始化函数（带自检）
// 说明：自检失败时会进入 while(1)
////////////////////////////////////////
void IAP_Init(void)
{
    uint8_t test_data[4] = {0x5A, 0xA5, 0x69, 0x96};
    uint8_t read_back[4];
    static uint8_t i;

    // 1. 配置 Flash 等待周期（关键！）
    WTST = IAP_WTST_VALUE;

    // 2. 自检流程：擦除 -> 写入 -> 验证 -> 擦除恢复

    // 2.1 擦除测试扇区
    EEPROM_SectorErase(IAP_TEST_ADDR);

    // 2.2 写入测试数据
    for (i = 0; i < 4; i++) {
        EEPROM_WriteByte(IAP_TEST_ADDR + i, test_data[i]);
    }

    // 2.3 读取并验证
    for (i = 0; i < 4; i++) {
        read_back[i] = EEPROM_ReadByte(IAP_TEST_ADDR + i);
        if (read_back[i] != test_data[i]) {
            // 自检失败，进入死循环等待调试
            // 查看变量：i = 失败的字节索引 (0~3)
            //          read_back[i] = 实际读取值
            //          test_data[i] = 期望值
            while (1);
        }
    }

    // 2.4 擦除恢复
    EEPROM_SectorErase(IAP_TEST_ADDR);

    // 自检通过
}

////////////////////////////////////////
// EEPROM 扇区擦除（外部接口）
// 入口参数: dwAddress - EEPROM 相对地址（0x0000 起始）
// 说明: 擦除包含该地址的整个扇区（512字节）
////////////////////////////////////////
void IAP_EraseSector(uint32_t dwAddress)
{
    EEPROM_SectorErase(dwAddress);
}

////////////////////////////////////////
// EEPROM 字节编程（外部接口）
// 入口参数: dwAddress - EEPROM 相对地址（0x0000 起始）
//           bData     - 待写入字节
// 说明: 必须先擦除后写入！
////////////////////////////////////////
void IAP_ProgramByte(uint32_t dwAddress, uint8_t bData)
{
    EEPROM_WriteByte(dwAddress, bData);
}

////////////////////////////////////////
// EEPROM 字节读取（外部接口）
// 入口参数: dwAddress - EEPROM 相对地址（0x0000 起始）
// 返回: 读取的字节数据
////////////////////////////////////////
uint8_t IAP_ReadByte(uint32_t dwAddress)
{
    return EEPROM_ReadByte(dwAddress);
}

////////////////////////////////////////
// 批量写入函数（基于单字节操作）
// 说明: 为了保持接口兼容，但内部使用逐字节方式
////////////////////////////////////////
void IAP_Write_16Bytes(uint32_t dwAddress, uint8_t far * dat)
{
    uint8_t i;
    for (i = 0; i < 16; i++) {
        EEPROM_WriteByte(dwAddress + i, dat[i]);
    }
}

void IAP_Write_4Bytes(uint32_t dwAddress, uint8_t far * dat)
{
    uint8_t i;
    for (i = 0; i < 4; i++) {
        EEPROM_WriteByte(dwAddress + i, dat[i]);
    }
}

void IAP_Read_16Bytes(uint32_t dwAddress, uint8_t far * dat)
{
    uint8_t i;
    for (i = 0; i < 16; i++) {
        dat[i] = EEPROM_ReadByte(dwAddress + i);
    }
}

void IAP_Read_4Bytes(uint32_t dwAddress, uint8_t far * dat)
{
    uint8_t i;
    for (i = 0; i < 4; i++) {
        dat[i] = EEPROM_ReadByte(dwAddress + i);
    }
}

void IAP_Read_2Bytes(uint32_t dwAddress, uint8_t far * dat)
{
    uint8_t i;
    for (i = 0; i < 2; i++) {
        dat[i] = EEPROM_ReadByte(dwAddress + i);
    }
}

// 以下对齐批量操作函数保留接口但标记为不推荐
// 如需使用，请确保地址对齐并理解其限制
void IAP_Write_16Bytes_Aligned(uint32_t dwAddress, uint8_t far * dat)
{
    IAP_Write_16Bytes(dwAddress, dat);
}

void IAP_Write_8Bytes_Aligned(uint32_t dwAddress, uint8_t far * dat)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        EEPROM_WriteByte(dwAddress + i, dat[i]);
    }
}

void IAP_Write_4Bytes_Aligned(uint32_t dwAddress, uint8_t far * dat)
{
    IAP_Write_4Bytes(dwAddress, dat);
}

void IAP_Read_16Bytes_Aligned(uint32_t dwAddress, uint8_t far * dat)
{
    IAP_Read_16Bytes(dwAddress, dat);
}

void IAP_Read_8Bytes_Aligned(uint32_t dwAddress, uint8_t far * dat)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        dat[i] = EEPROM_ReadByte(dwAddress + i);
    }
}

void IAP_Read_4Bytes_Aligned(uint32_t dwAddress, uint8_t far * dat)
{
    IAP_Read_4Bytes(dwAddress, dat);
}

void IAP_Read_2Bytes_Aligned(uint32_t dwAddress, uint8_t far * dat)
{
    IAP_Read_2Bytes(dwAddress, dat);
}
