#ifndef _DEBUG_ARCH_H
#define _DEBUG_ARCH_H

#include "stc32g144k246.h"  // 包含硬件寄存器定义
#include "dma_uart.h"

// 硬件 CRC16 计算 (MODBUS 标准)
static uint16_t Debug_Arch_CRC16(uint8_t *pData, uint16_t len) {
    uint16_t i;
    
    // 1. 初始化：RST=1(复位), MODE=0(CRC16)
    CRC_CR = 0x80; 
    
    // 2. 顺序写入数据
    for(i = 0; i < len; i++) {
        CRC_DIN = pData[i];
    }
    
    // 3. 读取结果 (STC32 硬件 CRC16 结果在 DOH 和 DOL)
    return ((uint16_t)CRC_DOH << 8) | CRC_DOL;
}

// 串口写入封装
static void Debug_Arch_Write(uint8_t *pData, uint16_t len) {
    UART5_Write((char *)pData, len);
}

// 串口读取封装
static uint16_t Debug_Arch_Read(uint8_t *pData, uint16_t len) {
    return UART5_Read((char *)pData, len);
}

#endif