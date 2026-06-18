/*---------------------------------------------------------------------*/
/* --- SPI Flash 通用驱动 (port 注入模式) ------------------------------*/
/*                                                                     */
/*  兼容芯片: PM25LV040 / W25X40CL / W25Q80BV / W25Q64BV / W25Q128FV   */
/*                                                                     */
/*  架构:                                                              */
/*    硬件层由 spi_flash_port_t 注入，驱动层不含任何 GPIO/寄存器操作      */
/*    DMA 可用时自动使用批量传输，否则回退到逐字节 txrx_byte               */
/*                                                                     */
/*---------------------------------------------------------------------*/

#include "spi_flash.h"

/* ================================================================
 * 内部状态变量
 * ================================================================ */
static const spi_flash_port_t *port = NULL;

static uint8_t flash_type   = FLASH_TYPE_NONE;
static uint8_t flash_id1    = 0;
static uint8_t flash_id     = 0;
static uint8_t flash_id2    = 0;

/* DMA 缓冲区 (必须放在 xdata，DMA 只能访问 xdata 空间) */
static uint8_t xdata tx_dma_buf[FLASH_DMA_BUF_SIZE];
static uint8_t xdata rx_dma_buf[FLASH_DMA_BUF_SIZE];

/* ================================================================
 * 内部辅助：DMA 路径是否可用
 * ================================================================ */
static uint8_t dma_available(void)
{
    uint8_t avail;

    avail = 0;
    if (port != NULL) {
        if (port->dma_xfer_start != NULL && port->dma_xfer_wait != NULL) {
            avail = 1;
        }
    }
    return avail;
}

/* ================================================================
 * 内部辅助：进入/退出临界区
 * ================================================================ */
static void crit_enter(void)
{
    if (port != NULL && port->enter_critical != NULL) {
        port->enter_critical();
    }
}

static void crit_exit(void)
{
    if (port != NULL && port->exit_critical != NULL) {
        port->exit_critical();
    }
}

/* ================================================================
 * 内部辅助：构建 DMA 发送命令头 (4 字节)
 * ================================================================ */
static void build_dma_header(uint8_t cmd, uint32_t addr)
{
    tx_dma_buf[0] = cmd;
    tx_dma_buf[1] = ((uint8_t *)&addr)[1];
    tx_dma_buf[2] = ((uint8_t *)&addr)[2];
    tx_dma_buf[3] = ((uint8_t *)&addr)[3];
}

/* ================================================================
 * 内部辅助：发送命令 + 3 字节地址 (非 DMA 路径用)
 * ================================================================ */
static void send_cmd_addr(uint8_t cmd, uint32_t addr)
{
    port->txrx_byte(cmd);
    port->txrx_byte(((uint8_t *)&addr)[1]);
    port->txrx_byte(((uint8_t *)&addr)[2]);
    port->txrx_byte(((uint8_t *)&addr)[3]);
}

/* ================================================================
 * 公开 API 实现
 * ================================================================ */

/******************************************************************************
 * @brief    注入移植层并初始化
 * @param    p   指向 spi_flash_port_t 的指针
 * @return   无
 ******************************************************************************/
void SPI_Flash_Init(const spi_flash_port_t *p)
{
    port = p;
    if (port != NULL && port->init != NULL) {
        port->init();
    }
}

/******************************************************************************
 * @brief    读取状态寄存器
 * @param    无
 * @return   状态寄存器值 (bit0 = WIP, 写正在进行中)
 ******************************************************************************/
uint8_t SPI_Flash_IsBusy(void)
{
    uint8_t dat;

    if (port == NULL) return 0;

    dat = 0;
    crit_enter();
    port->ce_low();
    port->txrx_byte(SFC_RDSR);
    dat = port->txrx_byte(0xFF);
    port->ce_high();
    crit_exit();

    return (dat & 0x01);
}

/******************************************************************************
 * @brief    带超时的忙等待
 * @param    timeout_ms   超时毫秒数，0 表示死等
 * @return   FLASH_OK 或 FLASH_TIMEOUT
 ******************************************************************************/
uint8_t SPI_Flash_WaitReady(uint16_t timeout_ms)
{
    uint16_t elapsed;

    if (port == NULL) return FLASH_ERROR;

    if (timeout_ms == 0) {
        /* 死等 — 兼容原版行为 */
        while (SPI_Flash_IsBusy());
        return FLASH_OK;
    }

    elapsed = 0;
    while (SPI_Flash_IsBusy()) {
        port->delay_ms(1);
        elapsed++;
        if (elapsed >= timeout_ms) {
            return FLASH_TIMEOUT;
        }
    }
    return FLASH_OK;
}

/******************************************************************************
 * @brief    识别 Flash 型号
 * @param    无
 * @return   FLASH_TYPE_xxx
 ******************************************************************************/
uint8_t SPI_Flash_CheckID(void)
{
    uint8_t id1;
    uint8_t id;
    uint8_t id2;

    if (port == NULL) return FLASH_TYPE_NONE;

    crit_enter();
    port->ce_low();
    port->txrx_byte(SFC_RDID);
    port->txrx_byte(0x00);      /* 3 个哑字节产生时钟 */
    port->txrx_byte(0x00);
    port->txrx_byte(0x00);
    id1 = port->txrx_byte(0xFF); /* 制造商 ID1 */
    id  = port->txrx_byte(0xFF); /* 设备 ID */
    id2 = port->txrx_byte(0xFF); /* 制造商 ID2 */
    port->ce_high();
    crit_exit();

    flash_id1 = id1;
    flash_id  = id;
    flash_id2 = id2;

    /* 识别芯片型号 */
    if ((id1 == 0x9D) && (id2 == 0x7F)) {
        flash_type = FLASH_TYPE_PM25LV040;
    } else if (id == 0x12) {
        flash_type = FLASH_TYPE_W25X40CL;
    } else if (id == 0x13) {
        flash_type = FLASH_TYPE_W25Q80BV;
    } else if (id == 0x16) {
        flash_type = FLASH_TYPE_W25Q64BV;
    } else if (id == 0x17) {
        flash_type = FLASH_TYPE_W25Q128FV;
    } else {
        flash_type = FLASH_TYPE_NONE;
    }

    return flash_type;
}

/******************************************************************************
 * @brief    获取芯片 ID 字节
 * @param    无
 * @return   设备 ID 字节
 ******************************************************************************/
uint8_t SPI_Flash_GetChipID(void)
{
    return flash_id;
}

/******************************************************************************
 * @brief    获取当前 Flash 型号
 * @param    无
 * @return   FLASH_TYPE_xxx
 ******************************************************************************/
uint8_t SPI_Flash_GetType(void)
{
    return flash_type;
}

/******************************************************************************
 * @brief    写使能
 * @param    无
 * @return   无
 ******************************************************************************/
void SPI_Flash_WriteEnable(void)
{
    if (port == NULL) return;
    if (flash_type == FLASH_TYPE_NONE) return;

    SPI_Flash_WaitReady(0);

    crit_enter();
    port->ce_low();
    port->txrx_byte(SFC_WREN);
    port->ce_high();
    crit_exit();
}

/******************************************************************************
 * @brief    整片擦除
 * @param    无
 * @return   FLASH_OK 或 FLASH_ERROR
 * @note     耗时较长 (几秒到几十秒)，调用后需等待 Busy 位清除
 ******************************************************************************/
uint8_t SPI_Flash_ChipErase(void)
{
    if (port == NULL) return FLASH_ERROR;
    if (flash_type == FLASH_TYPE_NONE) return FLASH_ERROR;

    SPI_Flash_WriteEnable();

    crit_enter();
    port->ce_low();
    port->txrx_byte(SFC_CHIPER);
    port->ce_high();
    crit_exit();

    return FLASH_OK;
}

/******************************************************************************
 * @brief    扇区擦除 (4KB)
 * @param    addr   扇区起始地址 (任意对齐，硬件按 4KB 对齐)
 * @return   FLASH_OK 或 FLASH_ERROR
 ******************************************************************************/
uint8_t SPI_Flash_SectorErase(uint32_t addr)
{
    uint8_t cmd;

    if (port == NULL) return FLASH_ERROR;
    if (flash_type == FLASH_TYPE_NONE) return FLASH_ERROR;

    SPI_Flash_WriteEnable();

    /* PM25LV040 使用不同的扇区擦除命令 */
    if (flash_type == FLASH_TYPE_PM25LV040) {
        cmd = SFC_SECTORER1;
    } else {
        cmd = SFC_SECTORER2;
    }

    crit_enter();
    port->ce_low();
    send_cmd_addr(cmd, addr);
    port->ce_high();
    crit_exit();

    return FLASH_OK;
}

/******************************************************************************
 * @brief    从 Flash 读取数据
 * @param    addr   起始地址
 * @param    buf    接收缓冲区指针
 * @param    len    读取长度 (0~65535)
 * @return   实际读取字节数
 * @note     内部自动跨 256 字节边界分段。
 *           DMA 可用时自动使用 DMA 批量传输。
 ******************************************************************************/
uint16_t SPI_Flash_Read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    uint8_t  i;
    uint16_t read_total;
    uint32_t cur_addr;
    uint16_t chunk;
    uint16_t dma_len;
    uint8_t *rx_ptr;

    if (port == NULL) return 0;
    if (buf == NULL) return 0;
    if (len == 0 || flash_type == FLASH_TYPE_NONE) return 0;

    SPI_Flash_WaitReady(0);

    read_total = 0;
    cur_addr = addr;

    while (read_total < len) {
        chunk = len - read_total;
        if (chunk > 256) {
            chunk = 256;
        }

        if (dma_available()) {
            /* ---- DMA 路径 ---- */
            dma_len = 4 + chunk;

            build_dma_header(SFC_READ, cur_addr);
            for (i = 4; i < dma_len; i++) {
                tx_dma_buf[i] = 0xFF;
            }

            crit_enter();
            port->ce_low();
            port->dma_xfer_start(tx_dma_buf, rx_dma_buf, dma_len, NULL);
            crit_exit();

            if (port->dma_xfer_wait(100) != FLASH_OK) {
                crit_enter();
                port->ce_high();
                crit_exit();
                return read_total;
            }

            crit_enter();
            port->ce_high();
            crit_exit();

            /* 从 rx_dma_buf + 4 拷贝有效数据 */
            rx_ptr = rx_dma_buf + 4;
            for (i = 0; i < chunk; i++) {
                buf[read_total + i] = rx_ptr[i];
            }
        } else {
            /* ---- 回退路径: 逐字节 txrx_byte ---- */
            crit_enter();
            port->ce_low();
            send_cmd_addr(SFC_READ, cur_addr);
            for (i = 0; i < chunk; i++) {
                buf[read_total + i] = port->txrx_byte(0xFF);
            }
            port->ce_high();
            crit_exit();
        }

        cur_addr += chunk;
        read_total += chunk;
    }

    return read_total;
}

/******************************************************************************
 * @brief    读取并逐字节比较
 * @param    addr   起始地址
 * @param    buf    待比较的数据缓冲区
 * @param    len    比较长度
 * @return   0 = 完全一致, 1 = 存在不一致, 2 = 参数错误
 * @note     DMA 可用时自动使用 DMA 批量读取后比较。
 ******************************************************************************/
uint8_t SPI_Flash_Compare(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    uint8_t  i;
    uint16_t cmp_total;
    uint32_t cur_addr;
    uint16_t chunk;
    uint16_t dma_len;
    uint8_t  mismatch;
    uint8_t *rx_ptr;

    if (port == NULL) return 2;
    if (buf == NULL) return 2;
    if (len == 0 || flash_type == FLASH_TYPE_NONE) return 2;

    SPI_Flash_WaitReady(0);

    mismatch = 0;
    cmp_total = 0;
    cur_addr = addr;

    while (cmp_total < len && mismatch == 0) {
        chunk = len - cmp_total;
        if (chunk > 256) {
            chunk = 256;
        }

        if (dma_available()) {
            /* ---- DMA 路径 ---- */
            dma_len = 4 + chunk;

            build_dma_header(SFC_READ, cur_addr);
            for (i = 4; i < dma_len; i++) {
                tx_dma_buf[i] = 0xFF;
            }

            crit_enter();
            port->ce_low();
            port->dma_xfer_start(tx_dma_buf, rx_dma_buf, dma_len, NULL);
            crit_exit();

            if (port->dma_xfer_wait(100) != FLASH_OK) {
                crit_enter();
                port->ce_high();
                crit_exit();
                return 2;
            }

            crit_enter();
            port->ce_high();
            crit_exit();

            rx_ptr = rx_dma_buf + 4;
            for (i = 0; i < chunk; i++) {
                if (buf[cmp_total + i] != rx_ptr[i]) {
                    mismatch = 1;
                    break;
                }
            }
        } else {
            /* ---- 回退路径 ---- */
            crit_enter();
            port->ce_low();
            send_cmd_addr(SFC_READ, cur_addr);
            for (i = 0; i < chunk; i++) {
                if (buf[cmp_total + i] != port->txrx_byte(0xFF)) {
                    mismatch = 1;
                    break;
                }
            }
            /* 如果中途 break，需要把剩余字节读完以完成 SPI 事务 */
            if (mismatch) {
                i++; /* 跳过已比较的那个 */
                for (; i < chunk; i++) {
                    port->txrx_byte(0xFF);
                }
            }
            port->ce_high();
            crit_exit();
        }

        cmp_total += chunk;
        cur_addr += chunk;
    }

    return mismatch;
}

/******************************************************************************
 * @brief    写数据到 Flash
 * @param    addr   起始地址
 * @param    buf    待写入数据缓冲区
 * @param    len    写入长度 (0~65535)
 * @return   实际写入字节数
 * @note     自动处理页边界 (256B)：在页边界处自动分段，
 *           每段重新等待就绪并写使能。
 *           目标区域需预先擦除 (扇区擦除后所有位为 1)。
 *           DMA 可用时自动使用 DMA 批量传输。
 ******************************************************************************/
uint16_t SPI_Flash_Write(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    uint8_t  i;
    uint16_t written;
    uint32_t cur_addr;
    uint16_t chunk;
    uint8_t  page_remaining;
    uint16_t dma_len;

    if (port == NULL) return 0;
    if (buf == NULL) return 0;
    if (len == 0 || flash_type == FLASH_TYPE_NONE) return 0;

    written = 0;
    cur_addr = addr;

    while (written < len) {
        /* 计算当前页剩余空间 (256 字节页边界) */
        page_remaining = 256 - (uint8_t)(cur_addr & 0xFF);
        chunk = len - written;
        if (chunk > page_remaining) {
            chunk = page_remaining;
        }

        /* 等待就绪并使能写 */
        SPI_Flash_WaitReady(0);
        SPI_Flash_WriteEnable();

        if (dma_available()) {
            /* ---- DMA 路径 ---- */
            dma_len = 4 + chunk;

            build_dma_header(SFC_PAGEPROG, cur_addr);
            for (i = 0; i < chunk; i++) {
                tx_dma_buf[4 + i] = buf[written + i];
            }

            crit_enter();
            port->ce_low();
            port->dma_xfer_start(tx_dma_buf, rx_dma_buf, dma_len, NULL);
            crit_exit();

            if (port->dma_xfer_wait(100) != FLASH_OK) {
                crit_enter();
                port->ce_high();
                crit_exit();
                return written;
            }

            crit_enter();
            port->ce_high();
            crit_exit();
        } else {
            /* ---- 回退路径 ---- */
            crit_enter();
            port->ce_low();
            send_cmd_addr(SFC_PAGEPROG, cur_addr);
            for (i = 0; i < chunk; i++) {
                port->txrx_byte(buf[written + i]);
            }
            port->ce_high();
            crit_exit();
        }

        cur_addr += chunk;
        written += chunk;
    }

    return written;
}
