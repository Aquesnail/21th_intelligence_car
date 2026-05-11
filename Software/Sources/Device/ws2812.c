#include "ws2812.h"
#include "pwm_user.h" // 包含AICube生成的pwm初始化函数声明

// 计算所需的缓冲区大小
// 每个灯24位，每位需要1个结构体(4字节)
// 额外加1个结构体用于发送结束时的低电平(Reset/Idle)
#define BUFFER_SIZE_PKTS   (WS2812_LED_NUM * 24 + 232)

// 定义DMA缓冲区 (放在 xdata 防止溢出)
WS2812_DMA_Packet_t xdata ws_buffer[BUFFER_SIZE_PKTS] = {0};

// 忙碌标志位
volatile bit ws_dma_busy = 0;

/**
 * @brief 驱动初始化
 */
void WS2812_Init(void)
{
    // 1. 调用底层的 PWM 初始化 (由 AICube 生成的那个函数)
    // 确保里面的 PWME_Init 已经包含了我们调试好的:
    // PWME_SetReload16(74); 分频1; 4字节突发; EnableUpdate;
    PWMC_Init(); 
    DMA_PWMC_EnableTxInt(); 
    // 2. 清空缓冲区（全0，即灯全灭）
    WS2812_Clear();
}

/**
 * @brief 设置单颗灯珠颜色
 * @param index 灯珠序号 (0 ~ LED_NUM-1)
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 */
void WS2812_SetColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t i;
    WS2812_DMA_Packet_t *p;
    
    if(index >= WS2812_LED_NUM) return;

    // 计算该灯珠在 buffer 中的起始位置
    // WS2812 数据格式顺序：G -> R -> B
    p = &ws_buffer[index * 24];

    // 填充 Green (8 bit)
    for(i = 0; i < 8; i++) {
        p->CCRxX = 0; p->Reserved = 0; p->CCRxH = 0;
        p->CCRxL = (g & (0x80 >> i)) ? WS_CODE_1 : WS_CODE_0;
        p++;
    }
    
    // 填充 Red (8 bit)
    for(i = 0; i < 8; i++) {
        p->CCRxX = 0; p->Reserved = 0; p->CCRxH = 0;
        p->CCRxL = (r & (0x80 >> i)) ? WS_CODE_1 : WS_CODE_0;
        p++;
    }
    
    // 填充 Blue (8 bit)
    for(i = 0; i < 8; i++) {
        p->CCRxX = 0; p->Reserved = 0; p->CCRxH = 0;
        p->CCRxL = (b & (0x80 >> i)) ? WS_CODE_1 : WS_CODE_0;
        p++;
    }
}

/**
 * @brief 清除所有灯珠颜色 (黑)
 */
void WS2812_Clear(void)
{
    uint16_t i;
    // 遍历所有数据包设置为 0码
    // 注意：这里设为 WS_CODE_0 还是 0?
    // 设为 0码 (18) 表示发送颜色(0,0,0)，灯会灭但保持协议信号。
    // 如果直接填0 (占空比0)，也是灭，但更省电。通常建议填 WS_CODE_0 以符合协议。
    for(i = 0; i < WS2812_LED_NUM * 24; i++) {
        ws_buffer[i].CCRxX = 0;
        ws_buffer[i].Reserved = 0;
        ws_buffer[i].CCRxH = 0;
        ws_buffer[i].CCRxL = WS_CODE_0; // 发送 "0" 数据位
    }
    
    // 最后一个包设为全0，确保总线拉低
    ws_buffer[WS2812_LED_NUM * 24].CCRxX = 0;
    ws_buffer[WS2812_LED_NUM * 24].Reserved = 0;
    ws_buffer[WS2812_LED_NUM * 24].CCRxH = 0;
    ws_buffer[WS2812_LED_NUM * 24].CCRxL = 0; 
}

/**
 * @brief 发送数据到 LED
 */
void WS2812_Show(void)
{
    // 等待上一次发送完成
    while(ws_dma_busy);
    
    ws_dma_busy = 1;

    // --- DMA 配置与启动 ---
    DMA_PWMC_DisableTx(); // 先关闭以配置
    
    // 设置地址 (强转为 uint8_t*)
    DMA_PWMC_SetTxAddress((uint8_t *)ws_buffer);
    
    // 设置传输总量：(灯数 * 24 + 1) * 4字节
    // 注意：STC的 SetTxAmount 通常接收的是“总字节数”或者“总次数”，
    // 根据你的库函数实现，如果是写入 DMA_AMT 寄存器，通常是 (次数-1)。
    // AICube生成的 SetTxAmount(val) 内部通常直接写寄存器，所以这里填 总字节数
    DMA_PWMC_SetTxAmount(BUFFER_SIZE_PKTS * 4); 
    
    DMA_PWMC_ClearTxFlag();
    DMA_PWMC_EnableTx();
    
    // 【关键】手动触发 (AICube可能未生成Trigger函数，手动置位)
    // 假设使用 PWMC，对应 DMA_PWMCR_CR 寄存器
    // 如果你有 DMA_PWMC_TriggerTx() 就用，没有就用下面的寄存器操作：
    // DMA_PWMCR_CR |= 0x40; // bit6 = TRIG
    DMA_PWMC_TriggerTx();
}

/**
 * @brief 查询是否忙碌
 */
uint8_t WS2812_IsBusy(void)
{
    return ws_dma_busy;
}

// ================= 中断处理 =================
// 放在 pwm.c 或者这里都可以，确保不要重复定义
// 如果 pwm.c 里已经有了 DMA_PWMETX_ISR，请把这段逻辑移过去

void DMA_PWMETX_ISR(void) interrupt DMA_PWMCT_VECTOR
{
    if (DMA_PWMC_CheckTxFlag())
    {
        DMA_PWMC_ClearTxFlag();
        
        // 传输完成，停止 DMA
        DMA_PWMC_DisableTx();
        
        // 强制通道输出低电平 (Reset 信号)
        // 这一步很重要，防止DMA停了但PWM还在按最后一个值输出
      //  PWM_UpdateDuty(PWMC_CH5, 0); 
        PWMC_SetCCR3Value(0); // 直接设置 CCR3 = 0，强制输出低电平
        
        // 清除忙标志
        ws_dma_busy = 0;
    }
}