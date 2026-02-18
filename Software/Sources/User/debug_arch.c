#include "debug_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h" // 务必在 FreeRTOSConfig.h 中启用 Event Groups
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


#pragma warning disable = 188
#pragma warning disable = 138

/* ================= 配置与定义 ================= */
#define MAX_DEBUG_VARS  16
#define TX_BUF_SIZE     128  // 统一发送缓冲区
#define RX_BUF_SIZE     128  // 接收缓冲区

// 协议常量
#define CMD_HANDSHAKE   0x00
#define CMD_WRITE       0x55
#define CMD_REGISTER    0x56
#define CMD_TEXT        0x57


// RTOS 事件位
#define EVT_CONNECTED   (1 << 0) // Bit 0: 上位机已连接

/* ================= 类型定义 ================= */

typedef struct {
    char name[11];      // 10 chars + \0
    uint8_t type;       // VarType_t
    void* addr;         // 4字节指针
} DebugObj_t;

/* ================= 静态变量 ================= */
static DebugObj_t debugTable[MAX_DEBUG_VARS];
static uint8_t debugVarCount = 0;
static uint8_t xdata txPkt[TX_BUF_SIZE]; // 统一发送缓冲
static char xdata print_buf[512];

/*================== 互斥锁   ================= */

SemaphoreHandle_t xLogMutex = NULL;

/*================== 信号量 =====================*/

SemaphoreHandle_t xDebugReadySingal = NULL;

// RTOS 句柄
static EventGroupHandle_t xDebugEventGroup = NULL;

/* ================= 外部回调 ================= */
extern void Debug_OnTextReceived(char* str, uint8_t len);

/* ================= 核心工具函数 ================= */
static uint16_t Debug_Arch_CRC16(uint8_t *pData, uint16_t len) {
    uint16_t crc_res;
    taskENTER_CRITICAL();
    CRC_CR = 0x83; 
    CRC_SetInitial16(0xFFFF);
    while(len--) {
        CRC_DIN = *pData++;
    }
    crc_res = ((uint16_t)CRC_DOH << 8) | CRC_DOL;
    taskEXIT_CRITICAL();
    return crc_res;
}

// 串口写入封装
static void Debug_Arch_Write(uint8_t *pData, uint16_t len) {
    UART5_Write((char *)pData, len);
}

// 串口读取封装
static uint16_t Debug_Arch_Read(uint8_t *pData, uint16_t len) {
    return UART5_Read((char *)pData, len);
}
// 获取类型长度
static uint8_t GetTypeSize(uint8_t type) {
    switch(type) {
        case DBG_UINT8: case DBG_INT8: return 1;
        case DBG_UINT16: case DBG_INT16: return 2;
        case DBG_UINT32: case DBG_INT32: case DBG_FLOAT: return 4;
        default: return 0;
    }
}

/**
 * @brief [核心精简] 通用发送函数
 * @param id   Packet ID (0x00-0x0F:变量, 0xFE:元数据, 0xFF:日志, 0xFD:握手)
 * @param type Packet Type (用于变量/日志类型，握手时填0)
 * @param payload 指向数据负载
 * @param len  负载长度
 */
static void Debug_TxCore(uint8_t id, uint8_t type, uint8_t *payload, uint8_t len) {
    uint16_t crc;
    uint8_t pktLen = 4 + len; // Head(1)+ID(1)+Type(1)+Len(1) + Payload(n)

    // 1. 组包头
    txPkt[0] = 0xAA;
    txPkt[1] = id;
    txPkt[2] = type;
    txPkt[3] = len;

    // 2. 拷贝负载
    if(len > 0 && payload != NULL) {
        memcpy(&txPkt[4], payload, len);
    }

    if(len > (TX_BUF_SIZE - 6)){
        len = TX_BUF_SIZE - 6;
    }
		
    // 3. 计算 CRC (范围: ID ~ Payload结束)
    crc = Debug_Arch_CRC16(&txPkt[1], pktLen - 1);

    // 4. 填充 CRC
    txPkt[pktLen]     = (uint8_t)(crc >> 8);
    txPkt[pktLen + 1] = (uint8_t)(crc & 0xFF);

    // 5. 发送
    Debug_Arch_Write(txPkt, pktLen + 2);
}

/* ================= 业务发送函数 ================= */

// 发送元数据 (ID=0xFE)
static void Debug_SendMetaPacket(uint8_t id) {
    uint8_t buf[16]; // 临时组装 [ID][Type][Addr4][Name10]
    if(id >= debugVarCount) return;

    buf[0] = id;
    buf[1] = debugTable[id].type;
    memcpy(&buf[2], &debugTable[id].addr, 4); // C251 4字节指针拷贝
    memset(&buf[6], 0, 10);
    strncpy((char*)&buf[6], debugTable[id].name, 10);
    if(xSemaphoreTake(xLogMutex,pdMS_TO_TICKS(10)) == pdTRUE){
        Debug_TxCore(0xFE, 0xFF, buf, 16);
        xSemaphoreGive(xLogMutex);
    }
    
}

// 发送变量值 (ID=0..N)
static void Debug_SendValuePacket(uint8_t id) {
    uint8_t len = GetTypeSize(debugTable[id].type);
    if(len > 0) {
        if(xSemaphoreTake(xLogMutex,pdMS_TO_TICKS(10)) == pdTRUE){
            Debug_TxCore(id, debugTable[id].type, (uint8_t*)debugTable[id].addr, len);
            xSemaphoreGive(xLogMutex);
        }
        
    }
}
static void Debug_SendHighFreqValuePacket(uint8_t id){
    uint8_t len = GetTypeSize(debugTable[id].type&DataType_Msk);
    if(len > 0) {
        if(xSemaphoreTake(xLogMutex,pdMS_TO_TICKS(10)) == pdTRUE){
            Debug_TxCore(id, debugTable[id].type, (uint8_t*)debugTable[id].addr, len);
            xSemaphoreGive(xLogMutex);
        }
        
    }
}
// 发送日志 (ID=0xFF)
void Debug_Log(const char* fmt,...) {
    va_list args;
    if(xLogMutex != NULL){
        if(xSemaphoreTake(xLogMutex,pdMS_TO_TICKS(100)) == pdTRUE){
            memset(print_buf,0x00,sizeof(print_buf));

            va_start(args,fmt);
            vsprintf(print_buf,fmt,args);
            va_end(args);
            Debug_TxCore(0xFF, DBG_STRING, (uint8_t*)print_buf, strlen(print_buf));

            xSemaphoreGive(xLogMutex);
        }else{
					while(1);
				}
        
    }
   
}
// 辅助函数：分片发送长字符串
// buffer: 待发送的长字符串（例如 CPU 统计信息）
void Debug_LogLongString(char *buffer) {
    char *pStart = buffer;
    char *pNext;
    
    while (*pStart != '\0') {
        // 寻找下一行
        pNext = strchr(pStart, '\n');
        
        if (pNext != NULL) {
            *pNext = '\0'; // 临时截断
            Debug_Log("%s\n", pStart); // 发送一行
            *pNext = '\n'; // 恢复
            pStart = pNext + 1; // 移动指针
        } else {
            Debug_Log("%s", pStart); // 发送最后一行
            break;
        }
        
        // 关键：给 DMA 和 RTOS 一点喘息时间
        // 如果不加延时，连续的 Debug_Log 可能会填满串口 RingBuffer 导致丢包
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

/* ================= 注册与接收逻辑 ================= */

// 注册变量
void Debug_Register(char* name, uint8_t type, void* addr) {
    if(debugVarCount < MAX_DEBUG_VARS) {
        memset(debugTable[debugVarCount].name, 0, 11);
        strncpy(debugTable[debugVarCount].name, name, 10);
        debugTable[debugVarCount].type = type;
        debugTable[debugVarCount].addr = addr;
        debugVarCount++;
    }
}

// [核心精简] 统一接收处理
static uint8_t xdata rxBuf[RX_BUF_SIZE];
static void Debug_ProcessIncoming(void) {
    
    uint16_t crc_calc, crc_recv;
    uint8_t cmd, len;
		uint8_t i;
		uint8_t id;
		uint8_t vLen;
        volatile uint8_t *ptr;
        //char logbuf[32];
    // 1. 预读头部 [0x55] [CMD] [LEN]
    if(Debug_Arch_Read(rxBuf, 3) == 3) {
        if(rxBuf[0] != 0x55) return;
        
        cmd = rxBuf[1];
        len = rxBuf[2];
        if(len > RX_BUF_SIZE - 5) return; // 保护

        // 2. 读取 [Payload...] + [CRC(2)]
        if(Debug_Arch_Read(&rxBuf[3], len + 2) == (len + 2)) {
            
            // 3. 校验 CRC (范围: CMD, LEN, Payload)
            crc_calc = Debug_Arch_CRC16(&rxBuf[1], len + 2);
            crc_recv = ((uint16_t)rxBuf[3+len] << 8) | rxBuf[4+len];

            if(crc_calc != crc_recv) return;

            // 4. 命令分发
            switch(cmd) {
                // ---> 握手请求 (Payload: 4 Bytes Magic)
                case CMD_HANDSHAKE:
                    if(len == 4) {
                        // 回复 Pong (ID=0xFD)
                        Debug_TxCore(0xFD, 0x00, &rxBuf[3], 4);
                        
                        // RTOS 事件组：标记连接并触发元数据发送
                        if((xEventGroupGetBits(xDebugEventGroup) & EVT_CONNECTED) == 0) {
                            xEventGroupSetBits(xDebugEventGroup, EVT_CONNECTED);
                            vTaskDelay(pdMS_TO_TICKS(10)); // 防止突发
                            // 首次连接，发送所有元数据
                            for(i=0; i<debugVarCount; i++) {
                                Debug_SendMetaPacket(i);
                                vTaskDelay(pdMS_TO_TICKS(5)); // 防止突发
                            }
                        }
                    }
                    break;

                // ---> 写变量 (Payload: ID, Len, Data...)
                case CMD_WRITE:
                    {
                        id = rxBuf[3]; 
                        vLen = rxBuf[4];
                        if(id < debugVarCount) {
                            // 直接内存拷贝修改
                            taskENTER_CRITICAL();
                            //memcpy(debugTable[id].addr, &rxBuf[5], vLen);
							ptr = (uint8_t *)(debugTable[id].addr);
                            ptr[0] = rxBuf[5];
                            if(vLen > 1) ptr[1] = rxBuf[6];
                            if(vLen > 2) ptr[2] = rxBuf[7];
                            if(vLen > 3) ptr[3] = rxBuf[8];
                            //sprintf(logbuf, "Wr Addr: %p", debugTable[id].addr); // %p 在 C251 会打印类型和偏移
						//	Debug_Log("Wr Addr: %p", debugTable[id].addr);
                            taskEXIT_CRITICAL();
                        }
                    }
                    break;

                // ---> 动态注册 (Payload: Type, Addr4, Name10)
                case CMD_REGISTER:
                    if(len == 15 && debugVarCount < MAX_DEBUG_VARS) {
                        debugTable[debugVarCount].type = rxBuf[3];
                        memcpy(&debugTable[debugVarCount].addr, &rxBuf[4], 4);
                        memset(debugTable[debugVarCount].name, 0, 11);
                        strncpy(debugTable[debugVarCount].name, (char*)&rxBuf[8], 10);
                        
                        // 注册成功，补发 Meta
                        //delay_ms(2);
                        debugVarCount++;//先增加计数器
                        Debug_SendMetaPacket(debugVarCount-1);
                        //delay_ms(2);
                        
                    }
                    break;

                // ---> 文本传输
                case CMD_TEXT:
                    rxBuf[3+len] = '\0'; // 安全截断
                    Debug_OnTextReceived((char*)&rxBuf[3], len);
                    break;
            }
        }
    }
}

/*================== 初始化 =================== */

void Debug_Init(){
    xLogMutex = xSemaphoreCreateMutex();
    xDebugReadySingal = xSemaphoreCreateBinary();
   // UART5_Init();
}


/* ================= 任务主体 ================= */
#define RECEIVE_FREQ 2
#define HIGH_FREQ 1
#define LOW_FREQ 10
void vDebugTask(void *pvParameters) {
    uint8_t i;
    //TickType_t xLastWakeTime;
    //TickType_t xLastWakeTimeHighFreq;
    //const TickType_t xFrequency = pdMS_TO_TICKS(100); // 10Hz 发送频率
    TickType_t t=0;
    (void)pvParameters;

    // 1. 初始化事件组
    xDebugEventGroup = xEventGroupCreate();
    xEventGroupClearBits(xDebugEventGroup, EVT_CONNECTED);

    // 初始化时间戳
    //xLastWakeTime = xTaskGetTickCount();
    //xLastWakeTimeHighFreq = xTaskGetTickCount();
    xSemaphoreGive(xDebugReadySingal);
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
        t++;
        // A. 快速处理接收指令 (Poll)
        if(t%RECEIVE_FREQ==0){
            Debug_ProcessIncoming();
            //Receive=0;
        }
        if (xEventGroupGetBits(xDebugEventGroup) & EVT_CONNECTED){
            if(t%HIGH_FREQ==0){
                //HighFreqTx=0;
                for(i = 0; i < debugVarCount; i++) {
                    if((debugTable[i].type&FreqType_Msk)!=0){
                        Debug_SendHighFreqValuePacket(i);
                    }
                }
            }
            if(t%LOW_FREQ==0){
               // LowFreqTx=0;
				//Debug_Log("log_test\r\n");
                for(i = 0; i < debugVarCount; i++) {
                    if((debugTable[i].type&FreqType_Msk)==0){
                        Debug_SendValuePacket(i);
                    }
                    // 给 DMA 一点点喘息时间，避免环形缓冲溢出
                    //vTaskDelay(pdMS_TO_TICKS(2)); 
                }
            }
        }
        // 这里的延时决定了对下行指令的响应速度
        // 20ms 的延时意味着每秒检查 50 次指令，足够流畅
        //vTaskDelay(pdMS_TO_TICKS(20)); 

        // B. 周期性发送数据 (Time Slicing)
        // 检查是否距离上次发送超过了 100ms
        // if ((xTaskGetTickCount() - xLastWakeTime) >= xFrequency) {
        //     xLastWakeTime = xTaskGetTickCount(); // 更新基准时间
        //     // 检查连接状态
        //     if (xEventGroupGetBits(xDebugEventGroup) & EVT_CONNECTED) {
        //         for(i = 0; i < debugVarCount; i++) {
        //             if((debugTable[i].type&FreqType_Msk)==0){
        //                 Debug_SendValuePacket(i);
        //             }
        //             // 给 DMA 一点点喘息时间，避免环形缓冲溢出
        //             vTaskDelay(pdMS_TO_TICKS(2)); 
        //         }
        //     }
        // }
        // if((xTaskGetTickCount() - xLastWakeTimeHighFreq >= 20 )){
        //     if (xEventGroupGetBits(xDebugEventGroup) & EVT_CONNECTED) {
        //         for(i = 0; i < debugVarCount; i++){
        //             if(debugTable[i].type&FreqType_Msk){
        //                 Debug_SendHighFreqValuePacket(i);
        //             }
        //         }
        //     }
        // }
    }
}