#include "debug_arch.h"
#include <string.h>
#include <stdio.h>

#pragma warning disable = 138

// 调试对象定义
typedef struct {
    char name[12];
    uint8_t type; 
    void* addr;
} DebugObj_t;

static DebugObj_t xdata debugTable[8];
static uint8_t debugVarCount = 0;
static uint8_t xdata txPkt[12]; // 增加到12字节以容纳 CRC16

void Debug_Register(char* name, uint8_t type, void* addr) {
    if(debugVarCount < 8) {
        strncpy(debugTable[debugVarCount].name, name, 11);
        debugTable[debugVarCount].type = type;
        debugTable[debugVarCount].addr = addr;
        debugVarCount++;
    }
}

/* --- 上行：发送带 CRC16 的二进制包 --- */
static void Debug_SendBinary(uint8_t id) {
    uint8_t i, len;
    uint16_t crc;
    uint8_t* pData = (uint8_t*)debugTable[id].addr;
    
    if(debugTable[id].type == 0) len = 1;
    else if(debugTable[id].type == 1) len = 2;
    else len = 4;

    txPkt[0] = 0xAA; // 帧头
    txPkt[1] = id;
    txPkt[2] = len;
    
    // 拷贝数据
    for(i = 0; i < len; i++) {
        txPkt[3+i] = pData[i];
    }

    // 计算 CRC16 (计算范围: 从 ID 到数据结束)
    crc = Debug_Arch_CRC16(&txPkt[1], 2 + len);
    
    txPkt[3+len] = (uint8_t)(crc >> 8);   // CRC 高位
    txPkt[4+len] = (uint8_t)(crc & 0xFF); // CRC 低位
    
    Debug_Arch_Write(txPkt, 5 + len);
}

/* --- 下行：处理带 CRC16 的修改指令 --- */
static void Debug_ProcessIncoming(void) {
    uint8_t rxBuf[12];
    uint16_t crc_calc, crc_recv;
    uint8_t id, len, i;
    uint8_t* pTarget;

    // 探测下行帧头 0x55
    if(Debug_Arch_Read(rxBuf, 1) > 0 && rxBuf[0] == 0x55) {
        // 读取 ID 和长度
        if(Debug_Arch_Read(&rxBuf[1], 2) == 2) {
            id = rxBuf[1];
            len = rxBuf[2];
            
            // 接收剩余负载 (Data + 2字节CRC)
            if(id < debugVarCount && Debug_Arch_Read(&rxBuf[3], len + 2) == (len + 2)) {
                // 计算 CRC
                crc_calc = Debug_Arch_CRC16(&rxBuf[1], 2 + len);
                crc_recv = ((uint16_t)rxBuf[3+len] << 8) | rxBuf[4+len];
                
                if(crc_calc == crc_recv) {
                    // 校验通过，写入内存
                    pTarget = (uint8_t*)debugTable[id].addr;
                    for(i = 0; i < len; i++) {
                        pTarget[i] = rxBuf[3+i];
                    }
                }
            }
        }
    }
}

void vDebugTask(void *pvParameters) {
    uint8_t i;
    char xdata msg[32];
    (void)pvParameters;

    // 宣告阶段
    Debug_Arch_Write((uint8_t*)"META_START\r\n", 12);
    for(i = 0; i < debugVarCount; i++) {
        sprintf(msg, "ID:%bu,N:%s,T:%bu\r\n", i, debugTable[i].name, debugTable[i].type);
        Debug_Arch_Write((uint8_t*)msg, strlen(msg));
    }
    Debug_Arch_Write((uint8_t*)"META_END\r\n", 10);

    for(;;) {
        for(i = 0; i < debugVarCount; i++) {
            Debug_SendBinary(i);
        }
        Debug_ProcessIncoming();
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}