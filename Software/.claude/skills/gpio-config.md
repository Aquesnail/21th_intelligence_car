# GPIO 引脚配置参考

本技能记录 STC32G12K128 GPIO 初始化完整配置，基于 [GPIO.md](GPIO.md) 功能分配表，用于查询引脚功能及变更指导。

---

## STC32G GPIO 寄存器速查

| 寄存器 | 功能 | 说明 |
|--------|------|------|
| PxM1, PxM0 | Mode | 00=准双向, 01=推挽, 10=高阻输入, 11=开漏 |
| PxPU | Pull-Up | 1=使能内部上拉电阻（弱上拉） |
| PxIE | Input Enable | 1=使能数字输入, 0=关闭(模拟输入) |
| PxNCS | No CMOS Schmitt | 1=关闭施密特, 0=使能(默认) |
| PxSR | Slew Rate | 1=慢摆率, 0=快摆率（高速信号） |
| PxDR | Drive | 1=增强驱动电流, 0=正常驱动 |
| PxINTE | Interrupt Enable | 1=使能引脚外部中断 |
| PINIPL/H | Priority | P0-P7 中断优先级 (00=最低, 11=最高) |
| PIN2IPL/H | Priority 2 | P8-PB 中断优先级 |

### 模式速查

| M1 | M0 | 模式 | 用途 |
|----|----|------|------|
| 0 | 0 | 准双向 | (本配置不再使用) |
| 0 | 1 | 推挽 | 输出引脚 (TX, MOSI, SCK, CS, PWM) |
| 1 | 0 | 高阻输入 | 输入引脚 / 未用引脚 |
| 1 | 1 | 开漏 | 需要电平转换/多主机共享的总线 |

---

## 完整引脚配置表

### P0 — UART

| 引脚 | 模式 | 上拉 | 速度 | 驱动 | 功能 |
|------|------|------|------|------|------|
| P0.0 | 开漏 | 弱上拉 | 慢 | 正常 | RX |
| P0.1 | 推挽 | 无 | **高速** | **增强** | TX |
| P0.2-P0.7 | 高阻 | 无 | — | — | 未用 |

```c
P0M1 = (PIN_0 | PIN_2 | PIN_3 | PIN_4 | PIN_5 | PIN_6 | PIN_7);  // 0xFD
P0M0 = (PIN_0 | PIN_1);          // 0x03
P0PU = PIN_0;                    // P0.0 弱上拉
P0SR = ~PIN_1;                   // P0.1 高速
P0DR = PIN_1;                    // P0.1 增强驱动
```

### P1 — 电磁输出检测输入

| 引脚 | 模式 | 功能 |
|------|------|------|
| P1.0 | 高阻 | ELECT_OUT_3 |
| P1.4 | 高阻 | ELECT_OUT_4 |
| P1.5 | 高阻 | ELECT_OUT_1 |
| P1.6 | 高阻 | ELECT_OUT_2 |
| 其余 | 高阻 | 未用 |

```c
P1M1 = 0xFF;
P1M0 = 0x00;
P1PU = 0x00;
```

### P2 — 全部高阻输入

```c
P2M1 = 0xFF;
P2M0 = 0x00;
P2PU = 0x00;
```

### P3 — IMU / MOTOR 中断输入 (RTOS 托管)

| 引脚 | 模式 | 上拉 | 功能 |
|------|------|------|------|
| P3.3 | 开漏 | 弱上拉 | SPI_IMU_INT2 |
| P3.4 | 开漏 | 弱上拉 | SPI_IMU_INT1 |
| P3.6 | 开漏 | 弱上拉 | SPI_MOTOR_INT2 |
| P3.7 | 开漏 | 弱上拉 | SPI_MOTOR_INT1 |
| 其余 | 高阻 | 无 | 未用 |

```c
P3M1 = 0xFF;
P3M0 = (PIN_3 | PIN_4 | PIN_6 | PIN_7);  // 0xD8
P3PU = (PIN_3 | PIN_4 | PIN_6 | PIN_7);  // 弱上拉
```

> **RTOS**: `configUSE_RTOS_P3 = 1`，[P3_ISR_Handle](Sources/Drivers/gpio_isr_handle.c) 统一处理四个中断源。

### P4 — 全部高阻输入

```c
P4M1 = 0xFF;
P4M0 = 0x00;
P4PU = 0x00;
```

### P5 — WS2812 LED PWM + 按键

| 引脚 | 模式 | 上拉 | 速度 | 驱动 | 中断 | 功能 |
|------|------|------|------|------|------|------|
| P5.0 | 推挽 | 无 | **高速** | **增强** | 无 | PWM_LED_WS2812 |
| **P5.4** | **开漏** | **弱上拉** | 慢 | 正常 | **下降沿** | **按键** |
| 其余 | 高阻 | — | — | — | 无 | 未用 |

```c
P5M1 = ~PIN_0;                    // 0xFE
P5M0 = (PIN_0 | PIN_4);           // 0x11
P5PU = PIN_4;                     // P5.4 弱上拉
P5SR = ~PIN_0;                    // P5.0 高速 (WS2812需要精确时序)
P5DR = PIN_0;                     // P5.0 增强驱动
P5IM2 &= ~PIN_4;                  // P5.4 下降沿触发
P5IM1 &= ~PIN_4;
P5IM0 &= ~PIN_4;
P5INTE = PIN_4;                   // P5.4 外部中断使能
```

### P6 — IMU SPI 接口

| 引脚 | 模式 | 上拉 | 速度 | 驱动 | 功能 |
|------|------|------|------|------|------|
| P6.4 | 推挽 | 弱上拉 | 慢 | 正常 | SPI_IMU_CS |
| P6.5 | 推挽 | 无 | **高速** | **增强** | SPI_IMU_MOSI |
| P6.6 | 开漏 | 弱上拉 | 慢 | 正常 | SPI_IMU_MISO |
| P6.7 | 推挽 | 无 | **高速** | **增强** | SPI_IMU_CLK |
| 其余 | 高阻 | 无 | — | — | 未用 |

```c
P6M1 = (PIN_0 | PIN_1 | PIN_2 | PIN_3 | PIN_6);  // 0x4F
P6M0 = (PIN_4 | PIN_5 | PIN_6 | PIN_7);           // 0xF0
P6PU = (PIN_4 | PIN_6);       // CS, MISO 弱上拉
P6SR = ~(PIN_5 | PIN_7);       // MOSI, CLK 高速
P6DR = (PIN_5 | PIN_7);        // MOSI, CLK 增强驱动
```

### P7 — MOTOR SPI 接口

| 引脚 | 模式 | 上拉 | 速度 | 驱动 | 功能 |
|------|------|------|------|------|------|
| P7.0 | 推挽 | 无 | 慢 | 正常 | SPI_MOTOR_CS |
| P7.1 | 推挽 | 无 | **高速** | **增强** | SPI_MOTOR_MOSI |
| P7.2 | 开漏 | 弱上拉 | 慢 | 正常 | SPI_MOTOR_MISO |
| P7.3 | 推挽 | 无 | **高速** | **增强** | SPI_MOTOR_CLK |
| 其余 | 高阻 | 无 | — | — | 未用 |

```c
P7M1 = (PIN_2 | PIN_4 | PIN_5 | PIN_6 | PIN_7);  // 0xF4
P7M0 = (PIN_0 | PIN_1 | PIN_2 | PIN_3);           // 0x0F
P7PU = PIN_2;                  // MISO 弱上拉
P7SR = ~(PIN_1 | PIN_3);       // MOSI, CLK 高速
P7DR = (PIN_1 | PIN_3);        // MOSI, CLK 增强驱动
```

### P8-PB — 全部高阻输入

```c
P8M1=0xFF; P8M0=0x00;
P9M1=0xFF; P9M0=0x00;
PAM1=0xFF; PAM0=0x00;
PBM1=0xFF; PBM0=0x00;
```

---

## 引脚功能速查（按功能排列）

| 功能 | 引脚 | 模式 | 备注 |
|------|------|------|------|
| UART RX | P0.0 | 开漏+上拉 | |
| UART TX | P0.1 | 推挽 高速高驱 | |
| ELECT_OUT_1 | P1.5 | 高阻输入 | |
| ELECT_OUT_2 | P1.6 | 高阻输入 | |
| ELECT_OUT_3 | P1.0 | 高阻输入 | |
| ELECT_OUT_4 | P1.4 | 高阻输入 | |
| SPI_IMU_INT1 | P3.4 | 开漏+上拉 | 外部中断输入 |
| SPI_IMU_INT2 | P3.3 | 开漏+上拉 | 外部中断输入 |
| SPI_MOTOR_INT1 | P3.7 | 开漏+上拉 | 外部中断输入 |
| SPI_MOTOR_INT2 | P3.6 | 开漏+上拉 | 外部中断输入 |
| SPI_IMU_CS | P6.4 | 推挽+上拉 | |
| SPI_IMU_MOSI | P6.5 | 推挽 高速高驱 | |
| SPI_IMU_MISO | P6.6 | 开漏+上拉 | |
| SPI_IMU_CLK | P6.7 | 推挽 高速高驱 | |
| SPI_MOTOR_CS | P7.0 | 推挽 | |
| SPI_MOTOR_MOSI | P7.1 | 推挽 高速高驱 | |
| SPI_MOTOR_MISO | P7.2 | 开漏+上拉 | |
| SPI_MOTOR_CLK | P7.3 | 推挽 高速高驱 | |
| PWM_LED_WS2812 | P5.0 | 推挽 高速高驱 | 需要精确时序 |
| 其他全部 | — | 高阻输入 | 未使用引脚 |

---

## 中断引脚

需要在 FreeRTOS 中托管的中断输入引脚（后续配置 FreeRTOS ISR 时使用）：

| 中断源 | 引脚 | 端口 |
|--------|------|------|
| SPI_IMU_INT1 | P3.4 | P3 |
| SPI_IMU_INT2 | P3.3 | P3 |
| SPI_MOTOR_INT1 | P3.7 | P3 |
| SPI_MOTOR_INT2 | P3.6 | P3 |

以上四个中断源均在 P3 口，可共用一个 P3 中断向量，在 ISR 中通过 `P3INTF` 区分具体引脚。

---

## 与原配置的主要变化

| 项目 | 旧配置 | 新配置 |
|------|--------|--------|
| UART | P1.0(RX), P1.1(TX) | P0.0(RX开漏), P0.1(TX推挽) |
| IMU SPI | P3.2-P3.5 | P6.4-P6.7 |
| MOTOR SPI | 无 | P7.0-P7.3 |
| IMU INT | P5.0, P5.1 | P3.3, P3.4 |
| MOTOR INT | 无 | P3.6, P3.7 |
| Buzzer | P6.0 | 移除 |
| LED PWM | P6.4 | P5.0 (WS2812) |
| ELECT_OUT | 无 | P1.0, P1.4-P1.6 |
| 未用引脚 | 多为准双向 | 全部高阻输入 |
