#ifndef _MOTOR_H
#define _MOTOR_H

#include "config.h"

extern uint8_t MOTOR_SPI_TxRx(uint8_t dat);           /* SPI单字节收发 */
extern void    MOTOR_SPI_DmaRead(uint8_t *rx_buf, uint16_t len);  /* SPI DMA读取 */
extern void    MOTOR_DelayMs(uint32_t ms);            /* 毫秒延时 */
extern uint32_t MOTOR_GetTick(void);                   /* 获取系统 tick */
extern void    MOTOR_Print(const char* fmt, ...);     /* 调试打印 */

#endif