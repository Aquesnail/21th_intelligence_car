# FreeRTOS 中断接管流程

本技能详细记录 STC32G12K128 平台上 FreeRTOS 接管硬件中断的完整流程，用于指导新增外设中断或调试中断相关问题。

---

## 概览：四层架构

```
┌──────────────────────────────────────────────────────────────┐
│ 1. 配置层    FreeRTOSConfig.h                                 │
│    configUSE_RTOS_xxx 宏 → 控制每个中断由 RTOS 还是裸机接管      │
├──────────────────────────────────────────────────────────────┤
│ 2. 向量重定向层   port_rtos.c (汇编 CSEG AT)                   │
│    在 MCU 中断向量地址放置 JMP → 跳转到 ISR 包装函数            │
├──────────────────────────────────────────────────────────────┤
│ 3. ISR 包装层   port_rtos.c (INSTALL_ISR_WRAPPER 宏)          │
│    保存/恢复上下文、判断是否需要任务切换                         │
├──────────────────────────────────────────────────────────────┤
│ 4. 用户处理层   各驱动文件 (*_user.c)                          │
│    实现实际的外设中断逻辑，接受 xHigherPriorityTaskWoken 参数    │
├──────────────────────────────────────────────────────────────┤
│ 底层支撑   portasm.h                                          │
│    汇编宏：上下文保存/恢复、栈规范化、任务切换、中断返回          │
└──────────────────────────────────────────────────────────────┘
```

---

## 第一步：配置中断接管模式

**文件**：[FreeRTOSConfig.h](Sources/Comm/FreeRTOSConfig.h)

为每个外设中断定义一个 `configUSE_RTOS_xxx` 宏：

```c
#define configUSE_RTOS_DMA_ADC2    0   // 裸机模式
#define configUSE_RTOS_DMA_SPI1    1   // RTOS 接管
#define configUSE_RTOS_DMA_SPI2    1
#define configUSE_RTOS_DMA_SPI3    0
#define configUSE_RTOS_P1          0   // 全高阻, 禁用
#define configUSE_RTOS_P2          0   // 全高阻, 禁用
#define configUSE_RTOS_P3          1   // IMU_INT1/2 + MOTOR_INT1/2
#define configUSE_RTOS_P4          0   // 全高阻, 禁用
#define configUSE_RTOS_P5          1   // P5.4 按键
#define configUSE_RTOS_P6          0   // 全高阻, 禁用
#define configUSE_RTOS_P8          0   // 全高阻, 禁用
```

- **值为 1**：中断由 FreeRTOS 完全接管，ISR 中可以安全调用 `xSemaphoreGiveFromISR()`、`xTaskNotifyFromISR()` 等 FreeRTOS API
- **值为 0**：中断保持裸机模式，RTOS 不做上下文保存/恢复，仅有 RETI+ERET 的最小返回

---

## 第二步：中断向量重定向（汇编）

**文件**：[port_rtos.c](Sources/FreeRTOS/portable/STC32G12K128/port_rtos.c) (文件底部)

STC32G12K128 的中断向量表位于固定地址。通过 `__asm { CSEG AT <vector_addr> }` 在对应向量地址放置一条 `JMP` 指令，跳转到 ISR 包装函数。

```c
// Timer0 中断 (Vector 0BH)
__asm { CSEG AT 0BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP vTimer0ISR?_ }     // LARGE 模式函数名后缀为 ?_
#else
    __asm { JMP vTimer0ISR?? }     // HUGE 模式函数名后缀为 ??
#endif
```

**注意事项**：
- 向量地址必须是 MCU 数据手册中定义的正确地址（如 `0BH` 是 Timer0，`027BH` 是 DMA_SPI2）
- `configUSE_ROMHUGE` 影响编译后函数名的修饰后缀（`?_` vs `??`），必须正确匹配
- 向量地址和 CSEG AT 使用十六进制，编译器通常接受 `0BH` / `0133H` 两种写法

### 已配置的中断向量对照表

| 外设      | 向量地址 | ISR 包装函数    | RTOS接管 | 用途 |
|-----------|----------|-----------------|----------|------|
| Timer0    | 0BH      | vTimer0ISR      | 1 | 系统 Tick |
| DMA_SPI1  | 018BH    | DMA_SPI1_ISR    | 1 | SPI1 DMA |
| DMA_SPI2  | 027BH    | DMA_SPI2_ISR    | 1 | SPI2 DMA |
| DMA_SPI3  | 0283H    | DMA_SPI3_ISR    | 0 | 裸机模式 |
| **P3**    | **0143H**| **P3_ISR**      | **1** | **IMU INT1/2 + MOTOR INT1/2** |
| P1        | 0133H    | P1_ISR          | 0 | 全高阻 (禁用) |
| P2        | 013BH    | P2_ISR          | 0 | 全高阻 (禁用) |
| P4        | 014BH    | P4_ISR          | 0 | 全高阻 (禁用) |
| P5        | 0153H    | P5_ISR          | 1 | P5.4 按键 |
| P6        | 015BH    | P6_ISR          | 0 | 全高阻 (禁用) |
| P8        | 016BH    | P8_ISR          | 0 | 全高阻 (禁用) |

---

## 第三步：ISR 包装宏

**文件**：[port_rtos.c](Sources/FreeRTOS/portable/STC32G12K128/port_rtos.c) (第 187-206 行)

`INSTALL_ISR_WRAPPER` 宏自动生成完整的 ISR 包装函数，根据 `configUSE_RTOS_xxx` 展开为两条路径：

```c
#define INSTALL_ISR_WRAPPER(__NAME, __HANDLE, __CONFIG) \
void __NAME##_ISR(void) { \
    if (__CONFIG == 1) { \
        /* ========== RTOS 接管路径 ========== */         \
        static BaseType_t xHPTaskWoken;                  \
        portNORMALIZE_INTERRUPT_STACK();                  \
        portSAVE_CONTEXT();                               \
        xHPTaskWoken = pdFALSE;                           \
        __HANDLE(&xHPTaskWoken);                          \
        if (xHPTaskWoken != pdFALSE) {                    \
            portEND_SWITCHING_ISR();                      \
        }                                                 \
        portRESTORE_CONTEXT();                            \
        portRELEASE_INTERRUPT();                           \
        portRETURN_TASK();                                \
    } else { \ 
        /* ========== 裸机路径 ========== */              \
        __HANDLE((BaseType_t *)0);                        \
        portRELEASE_INTERRUPT();                          \
        portRETURN_TASK();                                \
    }                                                     \
}
```

### RTOS 路径执行顺序

```
中断硬件入口
  │
  ▼
portNORMALIZE_INTERRUPT_STACK()   ← 将硬件中断栈帧规范化为 ECALL 格式
  │
  ▼
portSAVE_CONTEXT()                ← 压栈 DR56/DR28/DR24/DR20/DR16/DR12/DR8/DR4/DR0/PSW
  │
  ▼
用户处理函数 __HANDLE(&xHPTaskWoken)  ← 执行实际外设逻辑
  │
  ├─ xHPTaskWoken == pdTRUE ──► portEND_SWITCHING_ISR()
  │                                ├─ portSAVE_SPX()     保存当前任务栈指针到 TCB
  │                                ├─ vTaskSwitchContext() 选择下一个任务
  │                                └─ portRESTORE_SPX()  从新 TCB 恢复栈指针
  │
  ▼
portRESTORE_CONTEXT()             ← 弹栈恢复所有寄存器（此时已是新任务的栈）
  │
  ▼
portRELEASE_INTERRUPT()           ← 通过 RETI 清除中断优先级状态
  │
  ▼
portRETURN_TASK()                 ← POP PSW1 → SETB EA → ERET 返回任务执行
```

### 裸机路径执行顺序

```
中断硬件入口
  │
  ▼
__HANDLE(NULL)                    ← 直接执行用户逻辑，不保存 RTOS 上下文
  │
  ▼
portRELEASE_INTERRUPT()           ← 清除中断优先级
  │
  ▼
portRETURN_TASK()                 ← ERET 返回
```

---

## 第四步：编写用户中断处理函数

**文件**：各驱动文件（如 [spi2_user.c](Sources/Drivers/spi2_user.c)）

用户处理函数必须遵循以下签名：

```c
void XXX_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
```

- `xHigherPriorityTaskWoken`：在 RTOS 模式下指向一个 `BaseType_t` 变量，初始为 `pdFALSE`
- 如果在 ISR 中通过 `xSemaphoreGiveFromISR()`、`xTaskNotifyFromISR()` 等 API 解除了一个更高优先级任务的阻塞，该参数会被设置为 `pdTRUE`
- 在裸机模式下传入 `NULL`，用户代码应判断此参数是否为 NULL 再使用

### 典型实现示例

```c
void DMA_SPI2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken) {
    if (DMA_SPI2_CheckFlag()) {
        DMA_SPI2_ClearFlag();
        
        if (spi2_dma_callback != 0) {
            spi2_dma_callback(xHigherPriorityTaskWoken);
        }
        
        // 如果使用信号量通知任务：
        // if (xDmaSemaphore != NULL && xHigherPriorityTaskWoken != NULL) {
        //     xSemaphoreGiveFromISR(xDmaSemaphore, xHigherPriorityTaskWoken);
        // }
        
        dma_spi2_done = 1;
        spi2_dma_busy = 0;
    }
}
```

### 函数声明

用户处理函数必须在 [port_rtos.c](Sources/FreeRTOS/portable/STC32G12K128/port_rtos.c) 顶部用 `extern` 声明（第 176-185 行）：

```c
extern void DMA_SPI2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void DMA_SPI1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
// ... 其他声明
```

---

## 第五步：实例化 ISR

**文件**：[port_rtos.c](Sources/FreeRTOS/portable/STC32G12K128/port_rtos.c) (第 210-218 行)

使用 `INSTALL_ISR_WRAPPER` 宏实例化每个中断：

```c
INSTALL_ISR_WRAPPER(DMA_SPI2, DMA_SPI2_ISR_Handle, configUSE_RTOS_DMA_SPI2)
INSTALL_ISR_WRAPPER(DMA_SPI1, DMA_SPI1_ISR_Handle, configUSE_RTOS_DMA_SPI1)
INSTALL_ISR_WRAPPER(P1,       P1_ISR_Handle,       configUSE_RTOS_P1)
INSTALL_ISR_WRAPPER(P2,       P2_ISR_Handle,       configUSE_RTOS_P2)
INSTALL_ISR_WRAPPER(P3,       P3_ISR_Handle,       configUSE_RTOS_P3)
INSTALL_ISR_WRAPPER(P4,       P4_ISR_Handle,       configUSE_RTOS_P4)
INSTALL_ISR_WRAPPER(P5,       P5_ISR_Handle,       configUSE_RTOS_P5)
INSTALL_ISR_WRAPPER(P6,       P6_ISR_Handle,       configUSE_RTOS_P6)
INSTALL_ISR_WRAPPER(P8,       P8_ISR_Handle,       configUSE_RTOS_P8)
```

---

## 底层汇编宏详解

**文件**：[portasm.h](Sources/FreeRTOS/portable/STC32G12K128/portasm.h)

### portSAVE_CONTEXT() / portRESTORE_CONTEXT()

保存和恢复 CPU 上下文。压栈/弹栈顺序必须镜像对称：

```
压栈顺序: DR56 → DR28 → DR24 → DR20 → DR16 → DR12 → DR8 → DR4 → DR0 → PSW
弹栈顺序: PSW  → DR0  → DR4  → DR8  → DR12 → DR16 → DR20 → DR24 → DR28 → DR56
```

对应 `pxPortInitialiseStack()` 中任务栈初始化的顺序。

### portNORMALIZE_INTERRUPT_STACK()

硬件中断入口时，MCU 自动压栈的格式与 ECALL (ERET 返回) 不兼容。此宏将中断栈帧重组为 ECALL 兼容格式：

```
硬件中断自动压栈: [PC[23:16], PC[15:8], PC[7:0], PSW1]
重组为 ECALL 格式: [PC[23:16], PC[15:8], PC[7:0], PSW1]  ← 排列顺序不变但调整栈布局
```

操作：弹出 xPCH/xPCL/xPCB/PSW1 → 重新按 xPCB/xPCH/xPCL/PSW1 顺序压栈。

### portNORMALIZE_FUNCTION_STACK()（仅用于 vPortYield）

函数调用（LCALL/ECALL）的栈帧也需要规范化为 ECALL 格式。用于任务主动让出 CPU 时（`vPortYield`）。

### portSAVE_SPX() / portRESTORE_SPX()

将当前任务栈指针（DR60）保存到 TCB 的 `pxTopOfStack` 字段，或从中恢复：

```
SAVE:   DR60 → pxCurrentTCB->pxTopOfStack
RESTORE: pxCurrentTCB->pxTopOfStack → DR60
```

TCB 结构中的 `pxTopOfStack` 位于 TCB 偏移 +2 处（`@WR6+0x2`）。

### portEND_SWITCHING_ISR()

任务切换的核心序列：

1. `portSAVE_SPX()` — 保存当前任务栈指针
2. `vTaskSwitchContext()` — 选择下一个就绪任务，更新 `pxCurrentTCB`
3. `portRESTORE_SPX()` — 加载新任务的栈指针

调用后，`portRESTORE_CONTEXT()` 将从新任务的栈恢复寄存器。

### portRELEASE_INTERRUPT()

清除中断优先级状态触发器。通过执行一条 `RETI` 指令来告知 MCU 中断已处理完毕，但并不真正返回到中断前的地址（因为紧接着会执行 `ERET` 返回到任务上下文）：

```
CLR EA → PUSH PSW1 → PUSH 返回地址 → RETI
```

### portRETURN_TASK()

统一的任务返回出口。所有 ISR 和 `vPortYield` 最终都通过此宏返回：

```
POP PSW1 → SETB EA → ERET
```

---

## Tick 中断 (Timer0 ISR)

**文件**：[port_rtos.c](Sources/FreeRTOS/portable/STC32G12K128/port_rtos.c) (第 129-149 行)

系统节拍中断是独立实现的，不使用 `INSTALL_ISR_WRAPPER` 宏：

```c
void vTimer0ISR( void ) {
    UNUSED( pxCurrentTCB );
    portNORMALIZE_INTERRUPT_STACK();
    portSAVE_CONTEXT();

    #if configUSE_PREEMPTION == 1
        if( xTaskIncrementTick() != pdFALSE ) {
            portEND_SWITCHING_ISR();
        }
    #else
        xTaskIncrementTick();
    #endif

    portRESTORE_CONTEXT();
    portRELEASE_INTERRUPT();
    portRETURN_TASK();
}
```

**Timer0 硬件配置** (`prvPortSetupTimerInterrupt`, 第 153-171 行)：

- AUXR |= 0x80 — Timer0 工作于 1T 模式（不分频）
- TMOD 低 4 位清零 — 模式 0：16 位自动重载
- TM0PS = 44 — 预分频器设为 45（44+1）
- TL0/TH0 — 自动重载值 = 65536 − (MAIN_Fosc / 1000 / 45)

---

## 新增外设中断的完整步骤

1. **FreeRTOSConfig.h**：添加 `#define configUSE_RTOS_NEW_PERIPH  1`（或 0）

2. **驱动文件**：编写 `NEW_PERIPH_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken)` 函数
   - 需要调用 FreeRTOS API 时，通过 `xHigherPriorityTaskWoken` 参数传递
   - 同时兼容裸机模式（判断参数是否为 NULL）

3. **port_rtos.c**：
   - 顶部添加 `extern void NEW_PERIPH_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);`
   - 中间添加 `INSTALL_ISR_WRAPPER(NEW_PERIPH, NEW_PERIPH_ISR_Handle, configUSE_RTOS_NEW_PERIPH)`
   - 底部添加向量重定向：
     ```c
     __asm { CSEG AT <向量地址> }
     #if ( configUSE_ROMHUGE == 0 )
         __asm { JMP NEW_PERIPH_ISR?_ }
     #else
         __asm { JMP NEW_PERIPH_ISR?? }
     #endif
     ```

4. **编译验证**：确认 `configUSE_ROMHUGE` 设置与 ROM 模式一致

---

## 关键约束

- **编译器仅支持 C89**：局部变量必须放在函数开头声明
- **MCU 大端序**：多字节数据的高字节在低地址
- **中断嵌套**：`portNORMALIZE_INTERRUPT_STACK` 后 `EA` 保持关闭，`portRETURN_TASK` 中才重新 `SETB EA`
- **静态局部变量**：`xHPTaskWoken` 声明为 `static`，因为 ISR 包装函数可能被多次调用
