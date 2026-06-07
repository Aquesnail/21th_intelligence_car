#include "motor.h"
#include "timer.h"

#pragma warning disable = 47

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

static void motor_spi_send_u32_small(uint32_t val)
{
    uint8_t i;
		uint8_t byte_to_send;
    for (i = 0; i < MOTOR_FRAME_BYTES; i++) {
        // 计算当前应该发送哪个字节
        // 当 i=0 时，位移 24 位(如果是4字节)，发送最高位
        byte_to_send = (uint8_t)(val >> ((MOTOR_FRAME_BYTES - 1 - i) * 8));
        MOTOR_SPI_TxRx(byte_to_send);
    }
}

/* 通过 SPI 接收 32bit 值 (低字节在前) */
motor_data_t motor_spi_recv_u32(void)
{
    motor_data_t dat;
    int8_t i;
    dat.u32 = 0;
    for (i = 0; i < MOTOR_FRAME_BYTES; i++) {
        dat.u32 |= (uint32_t)MOTOR_SPI_TxRx(0xFFu) << (i * 8);
    }
    return dat;
}

/* 发送命令帧 */
static void motor_send_cmd(uint8_t cmd, uint8_t data1, uint8_t data2)
{
    motor_spi_send_u32(motor_pack_head(cmd, data1, data2));
}

/* 写单个寄存器: 发送命令帧 + 1个32bit数据 */
void motor_write_reg(uint8_t addr, motor_data_t value)
{
    motor_send_cmd(MOTOR_CMD_WRITE_ONE, addr, 0);
    motor_spi_send_u32_small(value.u32);
}

/* 写两个寄存器: 发送命令帧 + 2个32bit数据 (先addr1后addr2) */
void motor_write_two_regs(uint8_t addr1, uint8_t addr2, motor_data_t val1, motor_data_t val2)
{
    motor_send_cmd(MOTOR_CMD_WRITE_TWO, addr1, addr2);
    motor_spi_send_u32_small(val1.u32);
    motor_spi_send_u32_small(val2.u32);
}
/* 超时时间: 100ms = 100,000 ticks (Get_Tim9 为 1MHz) */
#define MOTOR_READ_TIMEOUT  100000UL


/* 读单个寄存器: 发送命令帧 -> 等待电平翻转 -> 接收1个32bit数据 */
motor_data_t motor_read_reg(uint8_t addr)
{
    motor_data_t dat;
    uint8_t tmp;
    tmp=P20;
    motor_send_cmd(MOTOR_CMD_READ_ONE,addr,addr);
    while(tmp!=P20){
        dat = motor_spi_recv_u32();
    }
    return dat;
}

/* 读单个寄存器 (带超时): 发送命令帧 -> 等待P20翻转 -> 接收1个32bit数据
 * timeout_ticks: 超时tick数, 0=无限等待
 * 超时时返回 dat.u32=0 */
motor_data_t motor_read_reg_timeout(uint8_t addr, uint32_t timeout_ticks)
{
    motor_data_t dat;
    uint32_t start_tick;
    uint8_t initial_p20;

    dat.u32 = 0;
    initial_p20 = P20;
    motor_send_cmd(MOTOR_CMD_READ_ONE, addr, addr);

    start_tick = MOTOR_GetTick();
    while (P20 == initial_p20) {
        if (timeout_ticks > 0) {
            if ((MOTOR_GetTick() - start_tick) >= timeout_ticks) {
                return dat;  /* 超时, 返回 0 */
            }
        }
    }
    /* P20 已翻转, 从机数据就绪 */
    dat = motor_spi_recv_u32();
    return dat;
}

/* 读两个寄存器: 发送命令帧 -> 等待电平翻转 -> 接收2个32bit数据 (先addr1后addr2) */
void motor_read_two_regs(uint8_t addr1, uint8_t addr2, motor_data_t *val1, motor_data_t *val2)
{
		return;
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
