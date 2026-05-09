#ifndef _MOTOR_H
#define _MOTOR_H

#include "config.h"

/* =========================================================
 * 底层接口 (由 motor_port.c 实现)
 * =========================================================*/
extern uint8_t  MOTOR_SPI_TxRx(uint8_t dat);
extern void     MOTOR_SPI_DmaRead(uint8_t *rx_buf, uint16_t len);
extern void     MOTOR_DelayMs(uint32_t ms);
extern uint32_t MOTOR_GetTick(void);
extern void     MOTOR_Print(const char* fmt, ...);

/* =========================================================
 * 协议 API
 * =========================================================*/
uint32_t motor_pack_head(uint8_t cmd, uint8_t data1, uint8_t data2);
void     motor_write_reg(uint8_t addr, uint32_t value);
void     motor_write_two_regs(uint8_t addr1, uint8_t addr2, uint32_t val1, uint32_t val2);
uint32_t motor_read_reg(uint8_t addr);
void     motor_read_two_regs(uint8_t addr1, uint8_t addr2, uint32_t *val1, uint32_t *val2);
void     motor_stream_start(uint8_t addr1, uint8_t addr2);
void     motor_stream_stop(void);
uint32_t motor_spi_recv_u32(void);
#endif