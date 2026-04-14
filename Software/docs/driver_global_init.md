# 全局变量初始化记录

本文档记录了对 `Sources/Drivers/`、`Sources/Apptest/` 与 `Sources/Device/` 下用户代码全局变量的显式初始化修改，避免未定义行为。

## Drivers

### adc_user.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `adc2_dma_buffer` | `uint8_t *` | `= NULL` |

### pwm.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `pu8PWMEDMATxBuffer` | `uint8_t xdata [12]` | `= {0}` |
| `TestBuffer` | `DMA_Burst4_Packet xdata [3]` | `= {0}` |

### dma_uart.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `uart4_dma_tx` | `static uint8_t xdata [512]` | `= {0}` |
| `uart4_dma_rx` | `static uint8_t xdata [512]` | `= {0}` |
| `uart4_ring_tx` | `static uint8_t xdata [2048]` | `= {0}` |
| `uart4_ring_rx` | `static uint8_t xdata [2048]` | `= {0}` |
| `uart4_mgr` | `dma_uart_manager_t` | `= {0}` |

### spi1_user.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `dma_spi1_done` | `volatile uint8_t` | `= 0` |
| `spi1_rx_buffer` | `static uint8_t *` | `= NULL` |
| `spi1_transfer_len` | `static uint8_t` | `= 0` |
| `spi1_dma_busy` | `uint8_t` | `= 0` |
| `pu8SPI1DMATxBuffer` | `uint8_t xdata [1024]` | `= {0}` |

### spi2_user.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `dma_spi2_done` | `volatile uint8_t` | `= 0` |
| `spi2_rx_buffer` | `static uint8_t *` | `= NULL` |
| `spi2_transfer_len` | `static uint8_t` | `= 0` |
| `spi2_dma_busy` | `uint8_t` | `= 0` |
| `pu8SPI2DMATxBuffer` | `uint8_t xdata [2048]` | `= {0}` |

## Apptest

### app_debug.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `debug_init_structure` | `debug_port_t` | `= {0}` |
| `echoBuf` | `char xdata [70]` | `= {0}` |

### app_main_control.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `test1` | `float` | `= 0.0f` |
| `test2` | `float` | `= 0.0f` |
| `test3` | `float` | `= 0.0f` |
| `SPI_DMA_Test` | `uint8_t xdata [16]` | `= {0}` |

## Device

### ws2812.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `ws_buffer` | `WS2812_DMA_Packet_t xdata [BUFFER_SIZE_PKTS]` | `= {0}` |

### beep.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `g_beep` | `static struct { ... }` | `= {0}` |
| `g_timer` | `static TimerHandle_t` | `= NULL` |

### icm42688.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `raw_data` | `static uint8_t xdata [14]` | `= {0}` |

### key_port.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `g_key_ids` | `uint32_t [5]` | `= {0}` |

### storage_mgr.c

| 变量 | 类型 | 初始化值 |
|------|------|----------|
| `g_ram_pkg` | `Package_t` | `= {0}` |
