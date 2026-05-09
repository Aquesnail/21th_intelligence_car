#include "motor.h"
#include "timer.h"

/* 命令码 */
#define MOTOR_CMD_WRITE_ONE        1u
#define MOTOR_CMD_WRITE_TWO        2u
#define MOTOR_CMD_READ_ONE         3u
#define MOTOR_CMD_READ_TWO         4u
#define MOTOR_CMD_STREAM_TRANSFER  5u
#define MOTOR_CMD_STOP             6u

/* SPI 单帧字节数 */
#define MOTOR_FRAME_BYTES          4u

/* 打包帧头: bit[7:0]=0xAA, bit[15:8]=cmd, bit[23:16]=data1, bit[31:24]=data2 */
uint32_t motor_pack_head(uint8_t cmd, uint8_t data1, uint8_t data2)
{
    return 0xAAu
         | ((uint32_t)cmd   << 8)
         | ((uint32_t)data1 << 16)
         | ((uint32_t)data2 << 24);
}

/* 通过 SPI 发送 32bit 值 (低字节在前) */
static void motor_spi_send_u32(uint32_t val)
{
    uint8_t i;
    for (i = 0; i < MOTOR_FRAME_BYTES; i++) {
        MOTOR_SPI_TxRx((uint8_t)(val & 0xFFu));
        val >>= 8;
    }
}

/* 通过 SPI 接收 32bit 值 (低字节在前) */
uint32_t motor_spi_recv_u32(void)
{
    uint32_t val;
    uint8_t i;
    val = 0;
    for (i = 0; i < MOTOR_FRAME_BYTES; i++) {
        val |= (uint32_t)MOTOR_SPI_TxRx(0xFFu) << (i * 8);
    }
    return val;
}

/* 发送命令帧 */
static void motor_send_cmd(uint8_t cmd, uint8_t data1, uint8_t data2)
{
    motor_spi_send_u32(motor_pack_head(cmd, data1, data2));
}

/* 写单个寄存器: 发送命令帧 + 1个32bit数据 */
void motor_write_reg(uint8_t addr, uint32_t value)
{
    motor_send_cmd(MOTOR_CMD_WRITE_ONE, addr, 0);
    motor_spi_send_u32(value);
}

/* 写两个寄存器: 发送命令帧 + 2个32bit数据 (先addr1后addr2) */
void motor_write_two_regs(uint8_t addr1, uint8_t addr2, uint32_t val1, uint32_t val2)
{
    motor_send_cmd(MOTOR_CMD_WRITE_TWO, addr1, addr2);
    motor_spi_send_u32(val1);
    motor_spi_send_u32(val2);
}
/* 超时时间: 100ms = 100,000 ticks (Get_Tim9 为 1MHz) */
#define MOTOR_READ_TIMEOUT  100000UL

/* 等待 P2.0 电平翻转, 超时返回1, 成功返回0 */
static uint8_t motor_wait_toggle(uint8_t pre_level)
{
    uint32_t start;
    start = Get_Tim9();
    while ((P2 & PIN_0) == pre_level) {
        if ((Get_Tim9() - start) >= MOTOR_READ_TIMEOUT) {
            return 1;
        }
    }
    return 0;
}

/* 读单个寄存器: 发送命令帧 -> 等待电平翻转 -> 接收1个32bit数据 */
uint32_t motor_read_reg(uint8_t addr)
{
    uint8_t pre_level;
    pre_level = (P2 & PIN_0);
    motor_send_cmd(MOTOR_CMD_READ_ONE, addr, 0);
    P22 = 1;
    if (motor_wait_toggle(pre_level)) {

        return 0;
    }
    P22 = 0;
    NOP30();
    return motor_spi_recv_u32();
}

/* 读两个寄存器: 发送命令帧 -> 等待电平翻转 -> 接收2个32bit数据 (先addr1后addr2) */
void motor_read_two_regs(uint8_t addr1, uint8_t addr2, uint32_t *val1, uint32_t *val2)
{
    uint8_t pre_level;
    pre_level = (P2 & PIN_0);
    motor_send_cmd(MOTOR_CMD_READ_TWO, addr1, addr2);
    if (motor_wait_toggle(pre_level)) {
        *val1 = 0;
        *val2 = 0;
        return;
    }
    *val1 = motor_spi_recv_u32();
    *val2 = motor_spi_recv_u32();
}

/* 开启流传输: 下位机自动连续推送 addr1 和 addr2 的当前值 */
void motor_stream_start(uint8_t addr1, uint8_t addr2)
{
    motor_send_cmd(MOTOR_CMD_STREAM_TRANSFER, addr1, addr2);
}

/* 停止流传输 */
void motor_stream_stop(void)
{
    motor_send_cmd(MOTOR_CMD_STOP, 0, 0);
}
