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
 * 联合体: 4字节数据的多种类型解读 (大端字节序)
 *   u8[0]=MSB, u8[3]=LSB; u16[0]=高半字, u16[1]=低半字
 * =========================================================*/
typedef union {
    uint32_t u32;
    float    f;
    uint16_t u16[2];
    uint8_t  u8[4];
} motor_data_t;

/* =========================================================
 * 协议 API
 * =========================================================*/
uint32_t motor_pack_head(uint8_t cmd, uint8_t data1, uint8_t data2);
void     motor_write_reg(uint8_t addr, motor_data_t value);
void     motor_write_two_regs(uint8_t addr1, uint8_t addr2, motor_data_t val1, motor_data_t val2);
void     motor_write_speed_targets(motor_data_t speed_r, motor_data_t speed_l);
motor_data_t motor_read_reg(uint8_t addr);
void         motor_read_two_regs(uint8_t addr1, uint8_t addr2, motor_data_t *val1, motor_data_t *val2);
void         motor_stream_start(uint8_t addr1, uint8_t addr2);
void         motor_stream_stop(void);
motor_data_t motor_spi_recv_u32(void);

#define MOTOR_CS_SET()   (P22 = 1)
#define MOTOR_CS_RESET() (P22 = 0)

#endif