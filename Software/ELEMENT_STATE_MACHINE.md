# 元素状态机与车辆控制全流程

> 适用于 STC32G 智能车，500Hz 主控循环，C89 编译器 + RTOS

---

## 目录

1. [系统架构概览](#1-系统架构概览)
2. [数据流：一个 Tick 的完整流水线](#2-数据流一个-tick-的完整流水线)
3. [核心数据结构](#3-核心数据结构)
4. [投票窗机制](#4-投票窗机制)
5. [元素状态机](#5-元素状态机)
6. [转移表](#6-转移表)
7. [元素属性表](#7-元素属性表)
8. [赛道顺序表（Track Table）](#8-赛道顺序表track-table)
9. [各元素详解](#9-各元素详解)
10. [超时机制](#10-超时机制)
11. [控制回路](#11-控制回路)
12. [上位机可调参数全表](#12-上位机可调参数全表)

---

## 1. 系统架构概览

```
┌──────────────────────────────────────────────────────────────┐
│                        硬件中断层                             │
│   P2.0 (电机SPI数据就绪) @500Hz  →  xTaskNotifyFromISR       │
│   ICM-42688 IMU (独立SPI任务)   →  Mahony AHRS               │
│   ADC2 DMA (4路电感)             →  循环采集                  │
└────────────────────────┬─────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│                  Main_Control_Task @500Hz                     │
│                                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────────┐  │
│  │ IMU姿态  │  │ ADC电感  │  │ 电机转速  │  │ 元素状态机   │  │
│  │ tilt_fb  │  │ adc[0..3]│  │ 实际速度  │  │ Element_     │  │
│  │ tilt_lr  │  │ 归一化   │  │ 低通滤波  │  │ Update()     │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └──────┬──────┘  │
│       │              │             │               │         │
│       │         ┌────▼────┐        │         查表3: speed,   │
│       │         │Err_Angle│        │         ctrl_strategy  │
│       │         │计算     │        │               │         │
│       │         └────┬────┘        │               │         │
│       │              │             │               │         │
│       │    ┌─────────▼─────────┐   │               │         │
│       │    │ 外环 Fuzzy P      │   │               │         │
│       │    │ + I (积分分离)    │   │               │         │
│       │    │ - D (不完全微分)  │   │               │         │
│       │    │ → 目标偏航角速度  │   │               │         │
│       │    └────────┬─────────┘   │               │         │
│       │             │             │               │         │
│       │    ┌────────▼────────┐    │               │         │
│       │    │ CTRL_OL_YAW?    │◄───┘               │         │
│       │    │ → 覆盖为开环角速度   │               │         │
│       │    └────────┬────────┘                    │         │
│       │             │                             │         │
│       │    ┌────────▼────────┐                    │         │
│       │    │ 内环 PI         │◄─── IMU陀螺仪Z轴   │         │
│       │    │ 目标 vs 实际    │                    │         │
│       │    │ → 差速          │                    │         │
│       │    └────────┬────────┘                    │         │
│       │             │                             │         │
│       │    ┌────────▼────────┐                    │         │
│       │    │ 速度合成         │◄── common_speed    │         │
│       │    │ L=基速-差速      │                    │         │
│       │    │ R=基速+差速      │                    │         │
│       │    └────────┬────────┘                    │         │
│       │             │                             │         │
│       │    ┌────────▼────────┐                    │         │
│       │    │ SPI 写电机       │                    │         │
│       │    └─────────────────┘                    │         │
└──────────────────────────────────────────────────────────────┘
```

**核心原则：控制线和检测线并行，互不阻塞。**

---

## 2. 数据流：一个 Tick 的完整流水线

`Element_Update()` 内部按以下顺序执行（行号参考 `element.c`）：

```
Section 0   安全检测层 (最高优先级)
            ├─ 4路ADC全低 > 300ms → SAFETY_STOP (急停)
            └─ SAFETY_STOP 下任一ADC恢复 → STRAIGHT (手动模式)

Section 0.5 赛道表
            └─ track_table_enabled==0 时复位 track_idx

Section 0.6 帧计数
            ├─ elem_frame_cnt++         (通用, 无条件)
            └─ ring_frame_cnt++         (仅环岛状态)

Section 1   倾角计算
            └─ tilt_fb_abs = |tilt_fb|

Section 2   投票生成 (7个投票窗并行投喂)
            ├─ vote_ring        (环岛入口/出口)
            ├─ vote_wall        (上墙入口)
            ├─ vote_wall_climb  (上爬→水平)
            ├─ vote_wall_horiz  (水平→下行)
            ├─ vote_wall_descend(下行→直道)
            ├─ vote_barrel      (滚筒进入/退出)
            └─ vote_cross       (十字进入/退出)

Section 4   状态转移评估
            └─ Element_EvaluateTransitions() → 遍历转移表
```

---

## 3. 核心数据结构

### 3.1 元素枚举 (14 个活跃元素)

| 值 | 枚举 | 含义 |
|---|---|---|
| 0 | `ELEMENT_STRAIGHT` | 直道正常循迹 |
| 1 | `ELEMENT_RING_LEFT_ENTRANCE` | 左环岛进入 |
| 2 | `ELEMENT_RING_RIGHT_ENTRANCE` | 右环岛进入 |
| 3 | `ELEMENT_WALL` | 上墙入口 |
| 4 | `ELEMENT_CROSS` | 十字路口 |
| 5 | `ELEMENT_BARREL` | 滚筒 |
| 6 | `ELEMENT_SKIP` | 跳过 (保留) |
| 7 | `ELEMENT_RING_LEFT_PASS` | 左环岛开环通过中 |
| 8 | `ELEMENT_RING_RIGHT_PASS` | 右环岛开环通过中 |
| 9 | `ELEMENT_IN_RING` | 环内行驶 |
| 10 | `ELEMENT_WALL_CLIMB` | 上墙: 上爬 |
| 11 | `ELEMENT_WALL_HORIZONTAL` | 上墙: 水平 |
| 12 | `ELEMENT_WALL_DESCEND` | 上墙: 下行 |
| 13 | `ELEMENT_SAFETY_STOP` | 安全停车 |

### 3.2 控制策略枚举

| 值 | 策略 | 行为 |
|---|---|---|
| 0 | `CTRL_NORMAL` | 正常PID循迹 → 写电机 |
| 1 | `CTRL_OL_YAW` | 开环角速度覆盖 → 写电机 |
| 2 | `CTRL_BRAKE` | 紧急刹车 → 写0 |
| 3 | `CTRL_FREEWHEEL` | 自由滑行 → 写0, 检测/LED照跑 |

### 3.3 投票窗 (VoteWindow_t)

```c
typedef struct {
    unsigned char *buf;       // 环形缓冲区
    unsigned char max_len;    // 缓冲区容量 (不可调)
    unsigned char idx;        // 写入位置
    unsigned char cnt;        // 窗口内 "1" 的个数 (O(1) 增量维护)
    unsigned char len;        // 有效窗长 (上位机可调, 0=旁路)
    unsigned char entry_thresh;  // 进入阈值: cnt >= 此值触发
    unsigned char exit_thresh;   // 退出阈值
} VoteWindow_t;
```

### 3.4 元素上下文 (ElementCtx_t)

```c
typedef struct {
    float adc[4];                     // 4路归一化电感
    float ax, ay, az;                 // 原始加速度计
    float yaw;                        // 展开的偏航角
    float tilt_fb, tilt_lr;           // 前后/侧倾角 (度)

    VoteWindow_t vote_ring;           // 7个投票窗
    VoteWindow_t vote_wall;
    VoteWindow_t vote_wall_climb;
    VoteWindow_t vote_wall_horiz;
    VoteWindow_t vote_wall_descend;
    VoteWindow_t vote_barrel;
    VoteWindow_t vote_cross;

    unsigned char  ring_side;         // 0=左环, 1=右环
    float          ring_entry_yaw;    // 入环时yaw快照
    unsigned short exit_lockout_cnt;  // 出环锁存倒计时
    unsigned short ring_frame_cnt;    // 环岛链专用帧计数
    unsigned short elem_frame_cnt;    // 通用元素帧计数

    unsigned char track_idx;          // 赛道表当前索引
    unsigned char track_table_enabled;

    element_t current;                // 当前/前一状态
    element_t prev;
} ElementCtx_t;
```

---

## 4. 投票窗机制

### 4.1 为什么需要投票窗

传感器有噪声，单帧信号可能抖动。投票窗用**多数表决**实现滞回——连续多帧满足条件才会触发状态转移。

### 4.2 工作原理

```
每帧 push 0 或 1:
  ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
  │ 1 │ 0 │ 1 │ 1 │ 0 │ 1 │ 1 │ 0 │ 1 │ 1 │  窗长=10, cnt=7
  └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
                                   ↓
                          cnt(7) >= thresh(7) → 触发!
```

- 每帧无论当前状态，feeder 都会 push（0 或 1），`len` 只增不减
- `len == 0` 表示上位机将窗长设为 0 → 旁路模式 → 直接读传感器
- `VoteWin_Reset` 清零缓冲区和 `cnt`/`idx`，但**不改 `len`**

### 4.3 7 个投票窗一览

| 窗口 | 缓冲区容量 | 默认窗长 | 进入阈值 | 退出阈值 | 用途 |
|---|---|---|---|---|---|
| vote_ring | 100 | 10 | 3 (30%) | 7 (70%) | 环岛入口+出口 |
| vote_wall | 20 | 10 | 7 (70%) | 7 | 上墙入口 |
| vote_wall_climb | 20 | 10 | 7 | 7 | 上爬→水平 |
| vote_wall_horiz | 20 | 10 | 7 | 7 | 水平→下行 |
| vote_wall_descend | 20 | 10 | 7 | 7 | 下行→直道 |
| vote_barrel | 20 | 10 | 7 | 7 | 滚筒进入+退出 |
| vote_cross | 20 | 10 | 7 | 7 | 十字进入+退出 |

---

## 5. 元素状态机

### 5.1 核心函数

| 函数 | 作用 |
|---|---|
| `Element_Update(ctx)` | 每帧调用一次，返回当前元素 |
| `Element_EvaluateTransitions(ctx)` | 遍历转移表，第一匹配即触发 |
| `Element_GetProp(e)` | 根据元素枚举查属性表 |
| `Element_GetEffectiveCtrl(e)` | 查控制策略（推车模式下覆写为 FREEWHEEL） |
| `Element_ResetTrackIndex(ctx)` | 复位赛道表索引（Safety Stop 时调用） |

### 5.2 转移触发流程

```
transition eval:
  for each entry in g_transitions[]:
    if (entry.from != ctx->current)  continue
    if (entry.check == NULL || entry.check(ctx)):
        ctx->current = entry.to
        entry.on_enter(ctx)          ← 执行 Enter 回调
        Debug_Log(name)
        break                        ← 匹配即停
```

### 5.3 Enter 回调约定

每个 Enter 函数的职责：

1. **清零对应投票窗** — 冲刷旧状态残留票
2. **复位帧计数器** (`elem_frame_cnt` 或 `ring_frame_cnt`) — 启动超时倒计时
3. **调用 `Enter_ExitToStraight`** — 如果目标是 STRAIGHT，推进赛道表

---

## 6. 转移表

转移表决定状态机的所有合法跳转。遍历时**按数组顺序，先匹配先触发**，`priority` 字段仅作标注。

```
共 22 条转移规则
```

### 6.1 无条件即时转移

| From | To | 说明 |
|---|---|---|
| RING_LEFT_ENTRANCE | RING_LEFT_PASS | 进入即跳 |
| RING_RIGHT_ENTRANCE | RING_RIGHT_PASS | 进入即跳 |
| WALL | WALL_CLIMB | 进入即跳 |

### 6.2 STRAIGHT → 各元素 (按优先级)

| From | To | Prio | Check | On Enter |
|---|---|---|---|---|
| STRAIGHT | RING_LEFT_ENTRANCE | 1 | `Check_RingLeftEntry` | `Enter_RingEntry` |
| STRAIGHT | RING_RIGHT_ENTRANCE | 1 | `Check_RingRightEntry` | `Enter_RingEntry` |
| STRAIGHT | WALL | 2 | `Check_WallEntry` | `Enter_Wall` |
| STRAIGHT | BARREL | 3 | `Check_BarrelEntry` | `Enter_Barrel` |
| STRAIGHT | CROSS | 4 | `Check_CrossEntry` | `Enter_Cross` |

### 6.3 环岛子状态链

| From | To | Prio | Check | On Enter |
|---|---|---|---|---|
| RING_LEFT_PASS | IN_RING | 0 | `Check_PassTimeout` | `Enter_InRing` |
| RING_RIGHT_PASS | IN_RING | 0 | `Check_PassTimeout` | `Enter_InRing` |
| IN_RING | STRAIGHT | 0 | `Check_RingExit` | `Enter_RingExit` |
| IN_RING | STRAIGHT | 1 | `Check_RingTimeout` | `Enter_RingTimeout` |

### 6.4 上墙子状态链

| From | To | Prio | Check | On Enter |
|---|---|---|---|---|
| WALL_CLIMB | WALL_HORIZONTAL | 0 | `Check_WallClimbToHoriz` | `Enter_WallHoriz` |
| WALL_HORIZONTAL | WALL_DESCEND | 0 | `Check_WallHorizToDescend` | `Enter_WallDescend` |
| WALL_DESCEND | STRAIGHT | 0 | `Check_WallDescendExit` | `Enter_ExitToStraight` |
| WALL_CLIMB | STRAIGHT | 1 | `Check_WallTimeout` | `Enter_WallTimeout` |
| WALL_HORIZONTAL | STRAIGHT | 1 | `Check_WallTimeout` | `Enter_WallTimeout` |
| WALL_DESCEND | STRAIGHT | 1 | `Check_WallTimeout` | `Enter_WallTimeout` |

### 6.5 滚筒 / 十字

| From | To | Prio | Check | On Enter |
|---|---|---|---|---|
| BARREL | STRAIGHT | 0 | `Check_BarrelExit` | `Enter_BarrelExit` |
| BARREL | STRAIGHT | 1 | `Check_BarrelTimeout` | `Enter_BarrelTimeout` |
| CROSS | STRAIGHT | 0 | `Check_CrossExit` | `Enter_CrossExit` |
| CROSS | STRAIGHT | 1 | `Check_CrossTimeout` | `Enter_CrossTimeout` |

---

## 7. 元素属性表

每个元素对应一条属性，控制线每帧查此表获取 `speed` 和 `ctrl` 策略：

| 元素 | 速度变量 | 默认值 | LED颜色 | 控制策略 | 开环设置 |
|---|---|---|---|---|---|
| STRAIGHT | `g_speed_cruising` | 50 | 白 (255,255,255) | CTRL_NORMAL | - |
| RING_LEFT_ENTRANCE | `g_speed_ring` | 50 | 蓝 (0,0,255) | CTRL_NORMAL | - |
| RING_RIGHT_ENTRANCE | `g_speed_ring` | 50 | 蓝 (0,0,255) | CTRL_NORMAL | - |
| WALL | `g_speed_wall` | 45 | 橙 (255,128,0) | CTRL_NORMAL | - |
| CROSS | `g_speed_cross` | 45 | 绿 (0,255,0) | CTRL_NORMAL | - |
| BARREL | `g_speed_barrel` | 45 | 紫 (255,0,255) | CTRL_NORMAL | - |
| SKIP | NULL | - | 黑 (0,0,0) | CTRL_NORMAL | - |
| RING_LEFT_PASS | `g_speed_ring` | 50 | 青 (0,255,255) | **CTRL_OL_YAW** | +3.0 °/s |
| RING_RIGHT_PASS | `g_speed_ring` | 50 | 青 (0,255,255) | **CTRL_OL_YAW** | -3.0 °/s |
| IN_RING | `g_speed_ring` | 50 | 黄 (255,255,0) | CTRL_NORMAL | - |
| WALL_CLIMB | `g_speed_wall_climb` | 45 | 橙 (255,128,0) | CTRL_NORMAL | - |
| WALL_HORIZONTAL | `g_speed_wall_horiz` | 40 | 黄 (255,255,0) | CTRL_NORMAL | - |
| WALL_DESCEND | `g_speed_wall_descend` | 50 | 青 (0,255,255) | CTRL_NORMAL | - |
| SAFETY_STOP | NULL | - | 红 (255,0,0) | CTRL_BRAKE | - |

---

## 8. 赛道顺序表（Track Table）

### 8.1 作用

防止传感器误识别：STRAIGHT 下只有**与表中期望元素匹配**的入口检测才会放行。

### 8.2 机制

```
Track_GetExpected() → 返回 g_track_table[g_track_idx]
                       索引超尾 → 返回 STRAIGHT (无约束)
                       表禁用 → 返回 g_single_element_debug

Track_CheckExpected(ctx, ELEMENT_WALL):
    return (Track_GetExpected() == ELEMENT_WALL) ? 1 : 0

Track_Advance():  g_track_idx++
    (在 Enter_ExitToStraight 或超时 Enter 中调用)
```

### 8.3 当前配置

```c
g_track_table[8] = { ELEMENT_WALL };
// 仅 WALL 一个活动条目，其余留空
```

---

## 9. 各元素详解

### 9.1 环岛 (Ring)

**状态链：** STRAIGHT → ENTRANCE → PASS → IN_RING → STRAIGHT

```
STRAIGHT
  │ Check_RingLeftEntry / Check_RingRightEntry
  │   ├─ Track_CheckExpected (赛道表把关)
  │   └─ Check_RingEntry_Physical:
  │       ├─ exit_lockout_cnt == 0 (出环锁存保护)
  │       ├─ VoteWin_Reached(vote_ring, entry_thresh=3)
  │       └─ 票源: adc[0]或adc[2] > RingEntrance[1200] AND |tilt_fb| < 15°
  ▼
ENTRANCE → (无条件) → PASS
                         │ Enter_RingPass: elem_frame_cnt=0, ring_frame_cnt=0
                         │ CTRL_OL_YAW 覆盖外环: 目标角速度 = ol_yaw_sign × 3.0°/s
                         │
                         │ Check_PassTimeout (150帧=300ms)
                         ▼
                       IN_RING
                         │ Enter_InRing: 记录 ring_entry_yaw, 复位 vote_ring
                         │ 恢复正常 PID 循迹
                         │
                         │ Check_RingExit:
                         │   ├─ yaw累计变化 > 200°
                         │   └─ VoteWin_Reached(vote_ring, exit_thresh=7)
                         │      票源: adc[0]或adc[2] > RingExit[1000]
                         ▼
                       STRAIGHT
                         │ Enter_RingExit: 启动 lockout_cnt=1000帧, 推进赛道表
                         │
                         └─ ring_frame_cnt >= 3000 → Check_RingTimeout → STRAIGHT
```

### 9.2 上墙 (Wall)

**状态链：** STRAIGHT → WALL → CLIMB → HORIZONTAL → DESCEND → STRAIGHT

```
STRAIGHT
  │ Check_WallEntry:
  │   ├─ Track_CheckExpected(WALL)
  │   └─ VoteWin_Reached(vote_wall, entry_thresh=7)
  │      票源: tilt_fb < -60°
  ▼
WALL → (无条件) → WALL_CLIMB
                    │ Enter_Wall: elem_frame_cnt=0, 复位全部4个vote窗
                    │
                    │ Check_WallClimbToHoriz:
                    │   票源: |tilt_lr| > 70°
                    ▼
                  WALL_HORIZONTAL
                    │ Enter_WallHoriz: 复位 vote_wall_horiz
                    │
                    │ Check_WallHorizToDescend:
                    │   票源: |tilt_lr| < 70°
                    ▼
                  WALL_DESCEND
                    │ Enter_WallDescend: 复位 vote_wall_descend
                    │
                    │ Check_WallDescendExit:
                    │   票源: tilt_fb > 60°
                    ▼
                  STRAIGHT (Track_Advance)
                    │
                    └─ 超时: 任一子状态 elem_frame_cnt >= 2000 → STRAIGHT
```

### 9.3 滚筒 (Barrel)

```
STRAIGHT
  │ Check_BarrelEntry:
  │   ├─ Track_CheckExpected(BARREL)
  │   └─ VoteWin_Reached(vote_barrel, entry_thresh=7)
  │      票源: |tilt_fb| > 25°
  ▼
BARREL
  │ Enter_Barrel: elem_frame_cnt=0, 复位 vote_barrel
  │
  │ Check_BarrelExit:
  │   VoteWin_Reached(vote_barrel, exit_thresh=7)
  │   票源: |tilt_fb| < 15° (滞回)
  ▼
STRAIGHT (Enter_BarrelExit: 复位 vote_barrel, Track_Advance)
  │
  └─ 超时: elem_frame_cnt >= 2000 → STRAIGHT
```

### 9.4 十字 (Cross)

```
STRAIGHT
  │ Check_CrossEntry:
  │   ├─ Track_CheckExpected(CROSS)
  │   └─ VoteWin_Reached(vote_cross, entry_thresh=7)
  │      票源: adc[1] + adc[3] > 5,020,000
  ▼
CROSS
  │ Enter_Cross: elem_frame_cnt=0, 复位 vote_cross
  │
  │ Check_CrossExit:
  │   VoteWin_Reached(vote_cross, exit_thresh=7)
  │   票源: adc[1] + adc[3] < 20,000 (滞回)
  ▼
STRAIGHT (Enter_CrossExit: 复位 vote_cross, Track_Advance)
  │
  └─ 超时: elem_frame_cnt >= 1000 → STRAIGHT
```

### 9.5 安全停车 (Safety Stop)

```
任意状态:
  │ 4路ADC全部 < 10, 持续 > 300ms
  ▼
SAFETY_STOP
  │ LED: 红色呼吸
  │ CTRL_BRAKE: 电机=0
  │
  │ 任一ADC恢复到 > 10
  ▼
STRAIGHT (手动模式, track_idx=0)
```

---

## 10. 超时机制

### 10.1 设计

超时建模为转移表中的**普通规则**（prio=1），排在正常转移（prio=0）之后，正常转移走不通时才触发。

### 10.2 超时一览

| 元素 | 超时阈值 | 默认值 | 计数器 | 挂载位置 |
|---|---|---|---|---|
| 上墙 (全子状态) | `g_wall_timeout_frames` | 2000 帧 (4s) | `elem_frame_cnt` | WALL_CLIMB/HORIZ/DESCEND → STRAIGHT |
| 滚筒 | `g_barrel_timeout_frames` | 2000 帧 (4s) | `elem_frame_cnt` | BARREL → STRAIGHT |
| 十字 | `g_cross_timeout_frames` | 1000 帧 (2s) | `elem_frame_cnt` | CROSS → STRAIGHT |
| 环岛 | `g_ring_timeout_frames` | 3000 帧 (6s) | `ring_frame_cnt` | IN_RING → STRAIGHT |

### 10.3 计数器生命周期

- `elem_frame_cnt`: 在 `Element_Update` 开头无条件 `++`。每个 Enter 函数（Enter_Wall/Barrel/Cross/RingPass）将其归零
- `ring_frame_cnt`: 仅在环岛状态（ENTRANCE/PASS/IN_RING）累加。`Enter_RingPass` 归零，`Enter_RingTimeout` 归零

---

## 11. 控制回路

### 11.1 Err_Angle 计算

```
Err_1 = adc[0] - adc[2]        (水平电感差, 左右)
Err_2 = adc[1] - adc[3]        (竖直电感差, 前后)
Err_3 = adc[0] + adc[2]        (水平电感和)
Err_4 = |adc[1] - adc[3]|     (竖直电感绝对差)

Err_Angle = (Err_A_K×Err_1 + Err_B_K×Err_2)
          / (Err_C_K×Err_3 + Err_D_K×Err_4) × 100

默认系数: A=2.0, B=6.0, C=3.0, D=3.0
```

### 11.2 外环：Fuzzy P + I - D → 目标偏航角速度

```
Err_Angle
  ├─ Slew-rate limiter (100°/s per tick)
  ├─ LPF (fc=100Hz)
  ├─ FuzzyPID_Step: 7模糊集自适应Kp → P输出
  ├─ + I项 (积分分离: 30 < |error| < 10000)
  ├─ - D项 (不完全微分 + 死区0.5 + MA滤波80窗)
  └─ → g_pid_output_filtered (目标偏航角速度, °/s)
```

### 11.3 开环覆盖

当 `Element_GetEffectiveCtrl() == CTRL_OL_YAW` (环岛PASS阶段):

```
g_pid_output_filtered = ol_yaw_sign × (*ol_yawrate_ptr)
                      = ±1 × 3.0 = ±3.0 °/s
```

外环 PID 被完全绕过。

### 11.4 内环：PI → 差速

```
differential_speed = PI_Step(
    inner_yaw_rate_pi,
    g_pid_output_filtered,     // 目标偏航角速度
    LPF(IMU_Gyro_Z, 100Hz)    // 实际偏航角速度
)
默认: Kp=5.5, Ki=0, Kd=0.1
```

### 11.5 速度合成

```
speed_L = common_speed - differential_speed
speed_R = -(common_speed + differential_speed)
```

`common_speed` 每帧从元素属性表的 `speed_ptr` 读取。

### 11.6 控制策略分发

| 策略 | 电机输出 |
|---|---|
| CTRL_NORMAL | `Motor_Write_Speeds(R, L)` |
| CTRL_OL_YAW | `Motor_Write_Speeds(R, L)` (角速度源不同) |
| CTRL_BRAKE | `Motor_Write_Speeds(0, 0)` |
| CTRL_FREEWHEEL | `Motor_Write_Speeds(0, 0)` |

---

## 12. 上位机可调参数全表

### 12.1 环岛参数

| 上位机名 | 变量 | 默认值 | 说明 |
|---|---|---|---|
| `RingEn0` | RingEntrance[0] | 1200 | 水平电感入口阈值 |
| `RingEn2` | RingEntrance[2] | 1200 | 水平电感入口阈值 |
| `RingEx0` | RingExit[0] | 1000 | 水平电感出口阈值 |
| `RingEx2` | RingExit[2] | 1000 | 水平电感出口阈值 |
| `Ring_AngThr` | angle_level | 15° | 车身水平倾角阈值 |
| `Ring_WinLen` | vote_ring.len | 10 | 环岛投票窗长 |
| `Ring_EntThr` | vote_ring.entry_thresh | 3 | 环岛入口票数阈值 |
| `OL_Yaw` | g_ol_ring_yawrate | 3.0 °/s | PASS开环角速度 |
| `PassTmOut` | g_pass_timeout_frames | 150 | PASS超时帧数 |
| `RingExYaw` | g_ring_exit_yaw_thresh | 200° | 出环所需yaw累计角 |
| `RingExLock` | g_ring_exit_lockout_frames | 1000 | 出环锁存帧数 |
| `Ring_Timeout` | g_ring_timeout_frames | 3000 | 环岛总超时帧数 |

### 12.2 上墙参数

| 上位机名 | 变量 | 默认值 | 说明 |
|---|---|---|---|
| `Wall_WinLen` | vote_wall.len | 10 | 上墙投票窗长 |
| `Wall_EntTiltB` | wall_entry_tiltb | -60° | 入墙前后倾角阈值 |
| `Wall_ClimbTR` | wall_climb_exit_tiltr | 70° | 爬升→水平侧倾阈值 |
| `Wall_HorizTR` | wall_horiz_exit_tiltr | 70° | 水平→下行侧倾阈值 |
| `Wall_DescTiltB` | wall_descend_exit_tiltb | 60° | 下行→直道前后倾阈值 |
| `Wall_Timeout` | g_wall_timeout_frames | 2000 | 上墙总超时帧数 |

### 12.3 滚筒参数

| 上位机名 | 变量 | 默认值 | 说明 |
|---|---|---|---|
| `Bar_EnTilt` | barrel_entry_tilt | 25° | 入筒倾角阈值 |
| `Bar_ExTilt` | barrel_exit_tilt | 15° | 出筒倾角阈值 |
| `Bar_Timeout` | g_barrel_timeout_frames | 2000 | 滚筒超时帧数 |

### 12.4 十字参数

| 上位机名 | 变量 | 默认值 | 说明 |
|---|---|---|---|
| `Cross_EnSum` | cross_entry_sum | 5,020,000 | 入十字电感和阈值 |
| `Cross_ExSum` | cross_exit_sum | 20,000 | 出十字电感和阈值 |
| `Cross_Timeout` | g_cross_timeout_frames | 1000 | 十字超时帧数 |
| `Cross_EntThr` | vote_cross.entry_thresh | 7 | 十字入口票数阈值 |
| `Cross_ExtThr` | vote_cross.exit_thresh | 7 | 十字出口票数阈值 |

### 12.5 速度参数

| 上位机名 | 变量 | 默认值 |
|---|---|---|
| `Cruising` | g_speed_cruising | 50 |
| `RingSpd` | g_speed_ring | 50 |
| `WallSpd` | g_speed_wall | 45 |
| `WClimbSpd` | g_speed_wall_climb | 45 |
| `WHorizSpd` | g_speed_wall_horiz | 40 |
| `WDescSpd` | g_speed_wall_descend | 50 |
| `BarrelSpd` | g_speed_barrel | 45 |
| `CrossSpd` | g_speed_cross | 45 |

### 12.6 控制与传感参数

| 上位机名 | 变量 | 默认值 | 说明 |
|---|---|---|---|
| `Err_K_A/B/C/D` | Err_A_K 等 | 2.0/6.0/3.0/3.0 | Err_Angle 计算系数 |
| `ADC_DeadZone` | adc_dead_zone | 5 | ADC死区 |
| `ADC_ErrZone` | adc_err_zone | 0.5 | 误差零化阈值 |
| `Fe_max` | pid_struct.e_max | 150 | FuzzyP 输入限幅 |
| `FKp0` | pid_struct.Kp0 | 0.063 | FuzzyP 基础Kp |
| `FKp(NB..PB)` | kp_singletons[7] | 2.1~1.0~2.1 | 7个模糊集Kp系数 |
| `Yaw_Kp/Ki/Kd` | inner PI gains | 5.5/0/0.1 | 内环偏航PI参数 |
| `Out_Ki` | g_outer_Ki | 0.23 | 外环积分系数 |
| `OutILim` | g_outer_i_limit | 3.6 | 外环积分限幅 |
| `OutIHi/Lo` | 积分分离阈值 | 10000/30 | |
| `Kd_Diff` | g_Kd_diff | 0 | 微分系数 |
| `DiffDead` | g_diff_deadband | 0.5 | 微分死区 |
| `D_MA_Win` | g_d_ma_window | 80 | 微分MA滤波窗长 |
| `OutSlew` | g_outer_slew_max | 100 | 外环 slew-rate 限幅 |
| `OutLPFFc` | g_outer_lpf_fc | 100 Hz | 外环LPF截止频率 |
| `GyroLPFFc` | g_gyro_lpf_fc | 100 Hz | 陀螺仪LPF截止频率 |
| `OffTkMs` | g_offtrack_timeout_ms | 300 ms | 离赛道超时 |
| `TblEnable` | g_track_table_enabled | 1 | 赛道表开关 |
| `DbgElem` | g_single_element_debug | 0 | 单元素调试目标 |
| `PushMode` | g_debug_push_mode | 0 | 推车调试开关 |

---

## 文件索引

| 文件 | 内容 |
|---|---|
| `Sources/User/element.h` | 元素枚举、结构体、API 声明 |
| `Sources/User/element.c` | 状态机、投票窗、转移表、属性表、超时 |
| `Sources/Apptest/app_main_control.c` | 主控任务、Err_Angle、FuzzyPID、PI、电机输出 |
| `Sources/User/application.c` | RTOS 任务创建 |
| `Sources/Comm/fuzzy_pid.c/h` | Fuzzy P 控制器 |
| `Sources/Comm/lpf.h` | 一阶 IIR 低通滤波器 |
| `Sources/Comm/moving_avg.h` | 滑动平均滤波器 |
| `Sources/User/imu_filter.c/h` | Mahony AHRS、四元数→欧拉角 |
| `Sources/Device/led_indicator.c/h` | LED 颜色映射、呼吸灯 |
