# spi_decode 使用文档

## 1. 功能简介

`spi_decode` 用于把 SPI 收到的 32-bit 指令帧解析成寄存器读写操作，并通过“地址 -> 变量”的元数据表访问实际变量。

- 支持命令：`read_one` / `read_two` / `write_one` / `write_two` / `streamtransfer` / `stop`
- 支持变量类型：`uint8_t`、`int8_t`、`uint16_t`、`int16_t`、`uint32_t`、`int32_t`、`float`
- 非法地址或越权（不可读/不可写）访问：静默丢弃，不回传错误码

实现文件：`user/command_ctl/endecode.hpp`

## 2. 对外接口

```cpp
explicit spi_decode(hw_flag_fifo<uint32_t>& scb_fifo_);

template <typename T>
bool register_map(uint8_t addr, T& value_ref, bool readable = true, bool writable = true);

bool unregister_map(uint8_t addr);
void clear_maps();
void trig();
```

说明：

- `register_map`：注册地址与变量绑定关系（可读可写权限可配置）
- `trig`：主处理入口，建议周期性调用（主循环或中断里单线程调用）

## 3. 快速接入

### 3.1 创建 FIFO 和解码器

```cpp
#include "cybsp.h"
#include "flag_fifo_SCB.hpp"
#include "endecode.hpp"

hw_flag_fifo<uint32_t> ctrl_fifo(SPI_CTR_HW);
spi_decode decoder(ctrl_fifo);
```

### 3.2 绑定寄存器地址到变量

```cpp
uint32_t motor_rpm_cmd = 0u;
float kp = 0.1f;
uint16_t status_word = 0u;

void ctl_register_init()
{
    decoder.register_map(0x01u, motor_rpm_cmd, true, true);   // 可读可写
    decoder.register_map(0x02u, kp, true, true);              // float
    decoder.register_map(0x03u, status_word, true, false);    // 只读
}
```

### 3.3 周期调用

```cpp
void ctl_poll()
{
    decoder.trig();
}
```

## 4. 协议格式

每帧是 32-bit，按位拆分如下（低位在前）：

- `head = raw[7:0]`，固定 `0xAA`
- `cmd  = raw[15:8]`
- `data1 = raw[23:16]`
- `data2 = raw[31:24]`

命令编码：

- `0`: `empty`
- `1`: `write_one`
- `2`: `write_two`
- `3`: `read_one`
- `4`: `read_two`
- `5`: `streamtransfer`
- `6`: `stop`

可用以下打包函数（上位机侧）：

```c
static inline uint32_t pack_head(uint8_t cmd, uint8_t data1, uint8_t data2)
{
    return 0xAAu |
           ((uint32_t)cmd << 8) |
           ((uint32_t)data1 << 16) |
           ((uint32_t)data2 << 24);
}
```

## 5. 命令使用示例

### 5.1 read_one

发送：

1. `pack_head(3, addr, 0)`

返回：

- 从 SPI 读回 1 个 `uint32_t` 值（该地址当前值）

### 5.2 read_two

发送：

1. `pack_head(4, addr1, addr2)`

返回：

- 先返回 `addr1` 值，再返回 `addr2` 值

### 5.3 write_one

发送：

1. `pack_head(1, addr, 0)`
2. `value`（32-bit 数据帧）

效果：

- 把 `value` 写入 `addr` 对应变量

### 5.4 write_two

发送：

1. `pack_head(2, addr1, addr2)`
2. `value1`
3. `value2`

效果：

- 按顺序写：`addr1 <- value1`，`addr2 <- value2`

### 5.5 streamtransfer

发送：

1. `pack_head(5, addr1, addr2)`

效果：

- 进入流传输模式，记录两个流地址 `addr1` 和 `addr2`。
- 之后每次调用 `trig()` 时会先检查输入 FIFO：
  - 如果该次 `trig()` 开始时输入 FIFO 为空，则自动向回传 FIFO 连续写入 `addr1` 和 `addr2` 对应的当前值。
  - 如果该次 `trig()` 开始时输入 FIFO 非空，则优先处理主机命令，本次不自动推送流数据。

### 5.6 stop

发送：

1. `pack_head(6, 0, 0)`（`data1/data2` 会被忽略）

效果：

- 退出流传输模式，停止自动推送。

## 6. 多类型按位透传规则

- `float`：读写都按 bit pattern 透传（IEEE754 位级一致）
- `uint8/int8/uint16/int16`：写入时取低位截断；读出时返回对应位宽的低位
- `uint32/int32`：按 32 位原样透传

`float` 主机端示例（C）：

```c
float f = 1.23f;
uint32_t bits;
memcpy(&bits, &f, sizeof(bits));     // 写入时发送 bits

uint32_t rx_bits = /* 从 SPI 收到 */;
float out;
memcpy(&out, &rx_bits, sizeof(out)); // 读回后还原 float
```

## 7. 行为与注意事项

- `register_map` 返回 `false` 常见原因：
  - 地址表已满（最多 64 项）
  - 传入空指针（内部保护）
- 若同一地址重复 `register_map`，会覆盖旧绑定。
- 当前实现在一次 `trig()` 内处理顺序是：
  1. 先解析所有输入帧
  2. 先执行读队列并回包
  3. 再执行写队列落地
  4. 若开启 `streamtransfer` 且本次 `trig()` 入口时输入 FIFO 为空，再自动推送 `data1/data2` 对应值

这意味着：同一次 `trig()` 中若同时有“先写后读同地址”请求，读可能先返回旧值。  
建议上位机把“写后读校验”分为两次交互周期，或确保固件下一轮 `trig()` 后再读。

- 软件 FIFO 深度为 30，突发命令过多时会覆盖最旧项；请确保 `trig()` 调用频率足够高。

## 8. 最小联调清单

1. 注册一个 `uint32_t` 地址，发送 `write_one` 后检查变量值变化。
2. 发送 `read_one`，确认回包与变量一致。
3. 注册一个 `float` 地址，写入位模式再读回，确认 bit 完全一致。
4. 把某地址设为只读，发送写命令，确认变量不变化且系统继续运行。
