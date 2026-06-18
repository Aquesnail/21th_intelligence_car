#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include "config.h"

/* ================================================================
 * SPI Flash 命令集
 * ================================================================ */
#define SFC_WREN        0x06    /* 写使能 */
#define SFC_WRDI        0x04    /* 写禁止 */
#define SFC_RDSR        0x05    /* 读状态寄存器 */
#define SFC_WRSR        0x01    /* 写状态寄存器 */
#define SFC_READ        0x03    /* 读数据 */
#define SFC_FASTREAD    0x0B    /* 快速读 */
#define SFC_RDID        0xAB    /* 读 ID (Release Power-down) */
#define SFC_PAGEPROG    0x02    /* 页编程 */
#define SFC_SECTORER1   0xD7    /* PM25LV040 扇区擦除命令 */
#define SFC_SECTORER2   0x20    /* W25Xxx 扇区擦除命令 */
#define SFC_BLOCKER     0xD8    /* 块擦除 (64KB) */
#define SFC_CHIPER      0xC7    /* 整片擦除 */

/* ================================================================
 * 识别的 Flash 型号
 * ================================================================ */
#define FLASH_TYPE_NONE         0
#define FLASH_TYPE_PM25LV040    1
#define FLASH_TYPE_W25X40CL     2
#define FLASH_TYPE_W25Q80BV     3
#define FLASH_TYPE_W25Q64BV     4
#define FLASH_TYPE_W25Q128FV    5

/* ================================================================
 * 返回码
 * ================================================================ */
#define FLASH_OK          0
#define FLASH_ERROR       1
#define FLASH_TIMEOUT     2

/* ================================================================
 * DMA 缓冲区最大长度 (4字节命令头 + 最多256字节页数据)
 * ================================================================ */
#define FLASH_DMA_BUF_SIZE  260

/* ================================================================
 * spi_flash_port_t — SPI Flash 底层移植接口
 *
 * 设计说明：
 *   1. txrx_byte — 同步单字节全双工，用于简短命令
 *       (Write Enable / Read Status / Chip Erase 等)
 *   2. dma_xfer_start / dma_xfer_wait — DMA 批量传输，用于读写数据
 *       若为 NULL 则自动回退到 txrx_byte 逐字节循环
 *   3. CE 控制独立于传输函数 — Flash 协议要求 CS 在整个
 *       命令+地址+数据 期间保持低电平，由 spi_flash.c 内部管理
 *   4. delay_ms — 用于超时等待，RTOS 下注入 vTaskDelay 包装，
 *       裸机下注入 delay_ms
 *   5. enter/exit_critical — 多任务安全，NULL 则不保护
 *
 * 使用方式：
 *   用户创建 spi_flash_port_t 实例，填充函数指针后调用
 *   SPI_Flash_Init(&port) 注入，之后所有 API 通过 port 操作硬件。
 * ================================================================ */

/* DMA 传输完成回调类型 */
typedef void (*spi_flash_dma_cb_t)(void);

typedef struct {
    /* ----- 必须：初始化 SPI 硬件 ----- */
    void    (*init)(void);

    /* ----- 必须：片选控制 ----- */
    void    (*ce_low)(void);
    void    (*ce_high)(void);

    /* ----- 必须：同步单字节全双工交换 ----- */
    uint8_t (*txrx_byte)(uint8_t tx_data);

    /* ----- 必须：毫秒延时 ----- */
    void    (*delay_ms)(uint16_t ms);

    /* ----- 可选：DMA 批量传输 (NULL 则回退到 txrx_byte) ----- */
    /* 启动 DMA：tx/rx 缓冲区由 spi_flash.c 提供，长度 <= FLASH_DMA_BUF_SIZE
       传输完成后在 ISR 中调用 cb（可为 NULL） */
    void    (*dma_xfer_start)(uint8_t *tx, uint8_t *rx, uint16_t len,
                              spi_flash_dma_cb_t cb);
    /* 阻塞等待 DMA 完成，返回 FLASH_OK 或 FLASH_TIMEOUT。
       timeout_ms=0 表示死等 */
    uint8_t (*dma_xfer_wait)(uint16_t timeout_ms);

    /* ----- 可选：临界区保护 (NULL 则不保护) ----- */
    void    (*enter_critical)(void);
    void    (*exit_critical)(void);

} spi_flash_port_t;

/* ================================================================
 * 公开 API
 * ================================================================ */

/* 注入移植层并初始化 */
void     SPI_Flash_Init(const spi_flash_port_t *port);

/* 识别 Flash 型号，返回 FLASH_TYPE_xxx */
uint8_t  SPI_Flash_CheckID(void);

/* 获取芯片 ID 字节 */
uint8_t  SPI_Flash_GetChipID(void);

/* 获取当前 Flash 型号 */
uint8_t  SPI_Flash_GetType(void);

/* 查询忙状态：返回状态寄存器 bit0 (WIP) */
uint8_t  SPI_Flash_IsBusy(void);

/* 带超时的忙等待，timeout_ms=0 则死等。
   返回 FLASH_OK 或 FLASH_TIMEOUT */
uint8_t  SPI_Flash_WaitReady(uint16_t timeout_ms);

/* 写使能 (内部自动等待 Flash 就绪) */
void     SPI_Flash_WriteEnable(void);

/* 整片擦除 (注意：耗时较长，几秒到几十秒) */
uint8_t  SPI_Flash_ChipErase(void);

/* 扇区擦除 (4KB) */
uint8_t  SPI_Flash_SectorErase(uint32_t addr);

/* 读取数据，返回实际读取字节数。
   内部自动跨 256 字节边界分段读取，DMA 可用时优先使用 DMA */
uint16_t SPI_Flash_Read(uint32_t addr, uint8_t *buf, uint16_t len);

/* 读取并逐字节比较，返回 0 表示一致，1 表示不一致，2 表示参数错误 */
uint8_t  SPI_Flash_Compare(uint32_t addr, const uint8_t *buf, uint16_t len);

/* 写数据，返回实际写入字节数。
   自动处理页边界 (256B)，在页边界处自动分段并重新使能写。
   DMA 可用时优先使用 DMA */
uint16_t SPI_Flash_Write(uint32_t addr, const uint8_t *buf, uint16_t len);

#endif /* __SPI_FLASH_H__ */
