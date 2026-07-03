# 21th 智能车 项目综览报告

> 生成日期: 2026-07-03

---

## 一、项目概述

| 项目 | 详情 |
|---|---|
| **MCU** | STC32G12K128 / STC32G144K246 (大端序) |
| **编译器** | Keil C251 (仅支持 **C89**) |
| **主频** | HIRC=48MHz → PLL → **120MHz** |
| **RTOS** | FreeRTOS V202112 (抢占式, 1ms tick) |
| **堆内存** | 6 KB (`configTOTAL_HEAP_SIZE`) |
| **最高优先级** | 32 级 |
| **机器人形态** | 双轮差速循迹小车，配备负压风扇吸附 |
| **开发工具** | Keil μVision 5 (`.uvproj`) |

---

## 二、目录结构

```
Software/
├── Sources/
│   ├── FreeRTOS/         # 内核源码 + STC32G 移植层
│   ├── Apptest/           # 应用层: 主控任务, 调试, 测试
│   │   ├── app_main_control.c  (1171行) ★ 核心: 500Hz 主控循环
│   │   ├── app_debug.c         # Debug 协议解析 + 命令处理
│   │   ├── app_uart_test.c     # UART 测试工具
│   │   └── app_clk_test.c      # 时钟测试
│   ├── User/              # 用户层: 核心算法 + 行为逻辑
│   │   ├── element.c      (1215行) ★ 元素状态机
│   │   ├── element.h      (151行)   # 状态机接口 + 数据结构
│   │   ├── imu_port.c     (330行)   ★ IMU 驱动 + FSYNC 时序对齐
│   │   ├── imu_filter.c   (172行)   ★ Mahony AHRS + 开机自校准
│   │   ├── motor_port.c   (60行)    # 电机 SPI 端口封装
│   │   ├── debug_arch.c   (~700行)  ★ Debug 双向通信协议栈
│   │   ├── cpu_status.c   (135行)   # CPU 使用率监控
│   │   ├── application.c/ # App 初始化 + 启动调度器
│   │   └── main.c         # 入口: 时钟/GPIO/外设初始化
│   ├── Device/            # 外设驱动层
│   │   ├── motor.c/h      # 电机 SPI 协议栈
│   │   ├── icm42688.c/h   # IMU 传感器 (SPI FIFO)
│   │   ├── ws2812.c/h     # RGB LED 驱动
│   │   ├── led_indicator.c/h  # LED 颜色映射
│   │   ├── key.c/key_port.c   # 按键处理
│   │   ├── beep.c         # 蜂鸣器
│   │   └── dma_uart_ring.c # DMA UART 环形缓冲
│   ├── Drivers/           # 底层硬件抽象层
│   │   ├── adc_user.c     # ADC2 DMA 采集 (4路电感)
│   │   ├── pwm.c/pwm_user.c  # PWM 输出 (风扇 + 电机信号)
│   │   ├── spi1/2/3_user.c   # 三路 SPI 驱动
│   │   ├── dma_uart.c     # DMA 串口
│   │   ├── gpio_user.c    # GPIO 初始化 + ISR
│   │   ├── clock.c        # 时钟树配置 (120MHz)
│   │   ├── delay.c        # us/ms 延时
│   │   ├── iap_user.c     # 片上 EEPROM/IAP
│   │   ├── mathlib.c      # 硬件数学加速库
│   │   ├── crc.c          # 硬件 CRC
│   │   └── timer.c        # 定时器驱动
│   ├── Comm/              # 通用算法库
│   │   ├── lpf.c/h        # 一阶 IIR 低通滤波器
│   │   ├── moving_avg.c/h # O(1) 环形移动平均
│   │   ├── fuzzy_pid.c/h  # 模糊 P 控制器 (7语言值)
│   │   ├── ringbuffer.c/h # 环形缓冲区
│   │   ├── simple_queue.c/h  # 简单队列
│   │   └── config.h       # 全局配置 (时钟/头文件)
│   └── AiCube/Sources/    # AiCube 辅助库
├── docs/                  # 文档: ELEMENT_STATE_MACHINE.md 等
├── Ref/                   # 参考代码
└── Listings/Objects/      # 编译产物
```

---

## 三、任务架构 (5个RTOS任务)

| 任务名 | 优先级 | 栈大小 | 周期 | 功能 |
|---|---|---|---|---|
| **IMU** | Idle+4 (最高) | 228B | 事件驱动 (1kHz FIFO IRQ) | ICM-42688-P 数据采集 + Mahony AHRS + FSYNC 时序对齐 |
| **main_control** | Idle+3 | 1224B | **500Hz** (P2.0 中断) | 主控循环: ADC→元素状态机→外环PID→内环PI→电机写入 |
| **vTaskMonitor** | Idle+2 | 512B | 3s | CPU 使用率 + 任务栈监控 |
| **Debug** | 由App_DebugTask_Init创建 | - | 事件驱动 | 上位机双向通信协议 |
| **IDLE** | 0 | 256B | - | FreeRTOS 空闲任务 |

---

## 四、主控数据流水线 (app_main_control.c, 500Hz)

```
P2.0 中断 (电机SPI就绪 @500Hz)
    │
    ▼
┌─────────────────────────────────────────────┐
│ Block 0: ADC 电感采集                       │
│   ADC2 DMA → 4路 adc_values[0..3]           │
│   → 归一化 (normalized_adc_k[4])            │
│   → 差比和差计算:                             │
│     Err_Angle = (k_A*Err1 + k_B*Err2) /     │
│                (k_C*Err3 + k_D*Err4) * 100  │
├─────────────────────────────────────────────┤
│ 元素状态机 Element_Update()                  │
│  ├─ 安全检测 (翻车/离赛道超时)                │
│  ├─ 投票窗 Push (环岛/上墙/滚筒/十字/直角)     │
│  ├─ 转移表评估 Element_EvaluateTransitions()  │
│  └─ SelectTargetErr() ← 直角冻结/正常        │
├─────────────────────────────────────────────┤
│ 劫持点: 参数集切换 (十字 vs 默认)               │
│   GetActiveParams() → ApplyParamsIfChanged() │
├─────────────────────────────────────────────┤
│ Block 1: 外环模糊PID + 积分分离 + 不完全微分  │
│   RunOuterLoop():                           │
│   1. Slew 斜率限制                           │
│   2. LPF 低通滤波 (可调 fc)                  │
│   3. 模糊P (7语言值 NB→PB)                   │
│   4. 积分分离 (可调 i_sep_hi/lo)             │
│   5. 陀螺仪 LPF → 不完全微分 → 自适应 Kd     │
│   → tar_yaw_raw (目标角速度)                  │
├─────────────────────────────────────────────┤
│ 劫持点: SelectTargetYaw()                    │
│   环岛PASS → 开环角速度覆盖                   │
├─────────────────────────────────────────────┤
│ Block 2: 内环 PI (角速度闭环)                 │
│   RunInnerLoop(): PI_Step()                 │
│   input: target_yaw vs 滤波后陀螺Z           │
│   → differential_speed (差速值)              │
├─────────────────────────────────────────────┤
│ 速度合成 → 电机写入                           │
│   L = common_speed - diff_speed              │
│   R = common_speed + diff_speed              │
│   → ControlDispatch() → Motor_Write_Speeds() │
└─────────────────────────────────────────────┘
```

---

## 五、元素状态机 (element.c)

### 5.1 状态枚举 (15种)

```
STRAIGHT → RING_LEFT_ENTRANCE/PASS/IN_RING  (环岛)
         → RING_RIGHT_ENTRANCE/PASS/IN_RING
         → WALL → WALL_CLIMB → WALL_HORIZONTAL → WALL_DESCEND  (上墙)
         → BARREL                                (滚筒)
         → CROSS                                 (十字)
         → RIGHT_ANGLE_LEFT / RIGHT_ANGLE_RIGHT  (直角)
         → SAFETY_STOP                           (紧急停车)
```

### 5.2 投票窗机制 (VoteWindow_t)

- **8 个独立投票窗**: 环岛(100B)、上墙/上爬/水平/下行(各20B)、滚筒(20B)、十字(20B)、直角(20B)
- **O(1) 增量计数**: 推入票时更新 cnt，无需遍历
- **进入/退出独立阈值**: 形成滞回，防止边界抖动
- **窗长可调 (上位机在线)**: 0=旁路(无滤波)

### 5.3 转移表 (g_transitions, 26条)

采用**优先级匹配**机制，按表顺序第一个满足条件的转移即触发：

| 源 → 目标 | 优先级 | 条件 |
|---|---|---|
| STRAIGHT → RING_LEFT/RIGHT | 1 | 环岛入口 (Track表 + 电感 + 水平) |
| STRAIGHT → WALL | 2 | TiltB < -60° |
| STRAIGHT → BARREL | 3 | \|TiltB\| > 25° |
| STRAIGHT → CROSS | 4 | 竖直电感和 > 阈值 |
| STRAIGHT → RIGHT_ANGLE | 5 | \|Err_Raw\| > 100 |
| WALL→CLIMB→HORIZ→DESCEND→STRAIGHT | 0 | 线性子状态链 |
| IN_RING → STRAIGHT | 0 | 出环检测 (yaw门禁) |
| 各超时兜底 | 1 | 帧计数超限 |

### 5.4 控制策略 (ControlStrategy_t)

| 策略 | 含义 | 电机行为 |
|---|---|---|
| `CTRL_NORMAL` | 正常 PID 循迹 | 写入差速控制量 |
| `CTRL_OL_YAW` | 开环角速度 (环岛 PASS) | 固定角速度转向 |
| `CTRL_BRAKE` | 紧急刹车 (SAFETY_STOP) | 速度=0, 风扇关闭 |
| `CTRL_FREEWHEEL` | 滑行 (推车调试) | 速度=0, 元素检测照常 |
| `CTRL_ERR_FREEZE` | 传感器冻结 (直角转弯) | 保留冻结时刻的 Err_Angle |

### 5.5 赛道顺序表 (Track Table)

- 8 元素顺序表，按赛道实际顺序约束元素进入
- 支持**单元素调试模式** (`track_table_enabled=0`)
- 每退出一个元素自动推进索引

---

## 六、控制算法详情

### 6.1 外环: 模糊P + 积分分离 + 不完全微分

```
输入 Err_Angle (差比和差结果)
  │
  ├─ Slew Rate 限制 → 防阶跃突变
  ├─ 一阶 IIR LPF (fc 可调, 默认100Hz)
  │
  ├─ 模糊 P 控制器:
  │   e 归一化到 [-1,1] → 7 三角形隶属度函数
  │   → 加权平均解模糊 → Kp = Kp0 × coeff
  │   → 7 个 Kp 系数表 (上位机可逐项调)
  │
  ├─ 积分分离:
  │   |e| > i_sep_hi → I=0 (大偏差不积分)
  │   |e| < i_sep_lo → I=0 (小偏差防止震荡)
  │   中间区域 → 正常积分 (clamp)
  │
  ├─ 不完全微分:
  │   差值 → 死区过滤 → 除以 dt → MA 滤波
  │
  └─ 自适应 Kd:
     |e| > Kd_diff_thresh → Kd_high (大偏差强微分)
     |e| ≤ Kd_diff_thresh → Kd_low  (小偏差弱微分)
```

### 6.2 内环: PI 角速度闭环

```
输入: target_yaw (角速度目标, °/s) vs gyro_filtered (实际角速度)
控制律: output = Kp·error + ∫Ki·error + Kd·(error - last_error)
参数: Kp=5.5, Ki=0.0, Kd≈0.1 (PI 模式, 微分极小)
```

### 6.3 速度结构

- **差速驱动**: `L = common_speed - diff, R = common_speed + diff`
- **缓启动 (Soft-Start)**: 加速度限制, 防止启动电流冲击
  - 加速步进: 2.0/帧
  - 减速步进: 10.0/帧 (刹车快速响应)
- **速度查表**: 每个元素独立速度 (直道50, 环岛50, 上墙45, 十字45 等)

---

## 七、传感器系统

### 7.1 IMU: ICM-42688-P (6轴)

- **接口**: SPI1 (DMA)
- **速率**: 1kHz FIFO 中断驱动
- **关键创新 - FSYNC 时序对齐**:
  - MCU 每 5 个采样周期发出 FSYNC 脉冲
  - 通过 `delta_mcu_time/delta_imu_time` 计算**时钟斜率补偿**
  - 一阶低通滤波 `imu_slope_k = new_k × 0.1 + old_k × 0.9`
  - 解决 MCU 与 IMU 晶振频率不匹配导致的积分误差
- **Mahony AHRS** (imu_filter.c):
  - 6轴传感器融合 (加速度+陀螺仪)
  - 输出: roll, pitch, yaw + 四元数
  - 重力向量 `(gx_body, gy_body, gz_body)` → 大倾角稳定
  - **开机自校准**: 2s 静止 → 陀螺仪零偏估计 (含抖动检测保护)
  - Yaw 解缠绕: 支持无限累积 `IMU_Get_Yaw_Unwrapped()`

### 7.2 ADC 电磁传感器

- **4路电感** (ADC2 DMA, 连续采集)
  - adc[0]: 右中横向
  - adc[1]: 竖直电感 (进环/十字用)
  - adc[2]: 左中横向
  - adc[3]: 竖直电感
- **自适应归一化**: 每通道独立系数, 支持 `norm0~norm3` 命令在线校准
- **死区**: `adc_dead_zone=5`
- **电感极性切换**: `change_inductor` 切换横/竖电感的差分极性

### 7.3 电机 (SPI3, 500Hz Stream)

- **协议**: 自定义 SPI 协议 (4字节帧, 0xAA 帧头)
- **功能**: 速度目标写入、PID参数读写、机械状态读取
- **Stream 模式**: 下位机自动连续推送 (速度A + 速度B + 机械状态)
- **12B/帧**: 通过 `motor_spi_dat_handle()` 大端序解析

---

## 八、调试系统 (debug_arch)

### 8.1 协议

- **物理层**: UART2 DMA
- **帧格式**: `[Header][Payload][CRC16]`
- **命令码**: 握手(0x00)、写变量(0x55)、注册(0x56)、日志(0x57)、静态刷新(0x58)、批量高频(0xFC)
- **性能**: 支持 >200 个变量注册, 高频/低频/静态三种采样策略

### 8.2 上位机可调参数统计

| 类别 | 数量 | 典型参数 |
|---|---|---|
| ADC/电感 | ~20 | 归一化系数, 死区, 误差权重K_A~K_D |
| 环岛 | ~15 | 进入/退出阈值, yaw门禁, 锁存帧数, 超时 |
| 上墙 | ~12 | 倾角阈值(tilt_b/tilt_r), 各子状态阈值 |
| 滚筒 | ~6 | 倾角阈值, 超时帧数 |
| 十字 | ~6 | 电感和阈值, 超时帧数 |
| 直角转弯 | ~6 | err阈值, 超时, 投票窗参数 |
| 外环PID | ~15 | Kp表(7个), Kd高低/Kd阈值, 积分分离边界, MA窗口, LPF截止 |
| 内环PI | ~6 | Kp/Ki/Kd, 积分限幅 |
| 速度 | ~9 | 各元素巡航速度 |
| 投票窗 | ~10 | 各窗窗长/进入阈值/退出阈值 |
| 电机PID | ~8 | A/B路Kp/Ki/Kd/ILim |
| 系统 | ~5 | 手动/自动, 缓启动步进, 驾驶速度 |

---

## 九、安全机制

| 机制 | 触发条件 | 行为 |
|---|---|---|
| 离赛道超时 | 4路 ADC 全低 > 300ms | → `ELEMENT_SAFETY_STOP`，刹车 |
| 上赛道超时 | `g_ontrack_timeout_tick > 0` 且总时间超标 | → `SAFETY_STOP` |
| 翻车检测 | 倾角异常 | → `SAFETY_STOP` |
| 手动模式看门狗 | 3s 无指令 | → 自动刹车 |
| 缓启动 | 速度阶跃 | → 加速度限制 |
| 环岛锁存 | 出环后 N 帧 | → 禁止再次进入 (防误触发) |
| 超时兜底 | 各元素独立超时 | → 强制退回 STRAIGHT |
| 模式切换清零 | 自动↔手动切换 | → PID/滤波器全量复位 |

---

## 十、代码质量观察

### 优点
1. **架构清晰**: 3段管线(外环→劫持→内环) + 转移表状态机, 职责分离好
2. **强可调性**: >150 个上位机在线调参变量, 覆盖几乎所有关键参数
3. **健壮性**: 多层看门狗(手动超时/元素超时/离赛道保护/锁存防抖/投票窗滞回)
4. **时序精度**: FSYNC 斜率补偿解决多晶振同步问题
5. **参数集分离**: 十字/默认两套独立参数, 切换时自动 seed 管线防突变
6. **代码自文档化**: 丰富的英文注释 + `ELEMENT_STATE_MACHINE.md` 文档

### 待改进点
1. **Motor_Reg_Read_Cb**: `switch` 缺省`default`分支存在但无实际处理
2. **`motor_read_reg`**: 空实现, 返回 0
3. **编码混用**: 部分注释为 GBK 乱码
4. **`extern` 跨文件引用**: `manual_control_flag`, `yaw` 变量等未通过头文件声明
5. **魔数残留**: 部分滤波器参数仍硬编码 (如 `imu_filter.c` 中 `Kp=1.2f, Ki=0.002f`)
6. **未使用代码**: 大量被注释掉的 `Debug_Register` 调用, 部分 `#if 0` 块
7. **堆栈分配**: IMU 任务栈仅 228B，存在溢出风险

---

## 十一、关键文件索引

| 文件 | 行数 | 角色 |
|---|---|---|
| [app_main_control.c](Sources/Apptest/app_main_control.c) | 1375 | **主控 500Hz 循环** — ADC→状态机→PID→电机 |
| [element.c](Sources/User/element.c) | 1215 | **元素状态机** — 转移表 + 投票窗 + 安全检测 |
| [element.h](Sources/User/element.h) | 151 | **状态机接口** — 枚举 + 上下文 + API |
| [imu_port.c](Sources/User/imu_port.c) | 330 | **IMU 驱动** — SPI FIFO + FSYNC 同步 |
| [imu_filter.c](Sources/User/imu_filter.c) | 172 | **Mahony AHRS** — 自校准 + 四元数融合 |
| [debug_arch.c](Sources/User/debug_arch.c) | ~700 | **调试协议栈** — 双向通信 |
| [fuzzy_pid.c](Sources/Comm/fuzzy_pid.c) | 155 | **模糊 P 控制器** — 7语言值解模糊 |
| [motor.c](Sources/Device/motor.c) | 119 | **电机 SPI 协议** — 读写/Stream |
| [ELEMENT_STATE_MACHINE.md](ELEMENT_STATE_MACHINE.md) | - | **系统设计文档** |

---

**总结**: 这是一个架构设计优秀的智能车竞赛项目，核心亮点在于**转移表状态机**的确定性行为、**FSYNC 时钟对齐**解决多晶振异步问题、以及**参数集分离+管线 seed**的平滑模式切换机制。代码在 C89 约束下保持了良好的模块化结构，通过 >150 个上位机在线参数实现了赛场快速调优能力。
