#ifndef __WS2812_DRIVER_H__
#define __WS2812_DRIVER_H__

#include "config.h" // 包含你的MCU头文件，如 AI8051U.h

// ================= 用户配置区 =================
#define WS2812_LED_NUM      4      // 修改这里：灯珠总数量
// ============================================

// 120MHz 主频下的参数 (ARR=74)
// 0码: ~300ns (18/75 * 1.25us)
// 1码: ~750ns (45/75 * 1.25us)
#define WS_ARR_VAL          74
#define WS_CODE_0           18
#define WS_CODE_1           45

// 核心结构体：DMA每次搬运4字节，分别对应 CCR5X, Res, CCR5H, CCR5L
typedef struct {
    uint8_t CCRxX;      // 对应 CCRnX
    uint8_t Reserved;   // 保留位
    uint8_t CCRxH;      // 对应 CCRnH
    uint8_t CCRxL;      // 对应 CCRnL (实际占空比)
} WS2812_DMA_Packet_t;

// 函数声明
void WS2812_Init(void);
void WS2812_SetColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void WS2812_Clear(void);
void WS2812_Show(void);
uint8_t WS2812_IsBusy(void);

#endif