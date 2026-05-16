# Bug: 高优先级任务永不阻塞，饿死所有低优先级任务

**文件**: `Sources/Apptest/app_main_control.c`
**函数**: `Motor_Data_Task`
**日期**: 2026-05-15

## 现象

Motor_Data_Task 卡住所有其他任务（包括 Main_Control_Task），系统无法正常运行。

## 根因

`xTaskNotifyWait` 的 `ulBitsToClearOnEntry` 参数为 `0`，导致进入等待前不清除已积压的通知位。

```
xTaskNotifyWait(0, SIGNAL_MOTOR_DATA_READY, &ulNotifiedValue, portMAX_DELAY)
//              ↑
//         不清除积压 → 如果 SPI 读取期间来了新中断，通知位已置位，立即返回
```

时序：

```
500Hz ISR → 置位 → Task 唤醒 → 清位 → 读 SPI (3×4字节, ~1.5ms)
                                            │
                     新 ISR 置位 (2ms周期) ←─┘ SPI 还没读完
                         │
Task 读完 → xTaskNotifyWait → 位已置 → 立即返回 → 再次读 SPI → 永不阻塞
```

结果：任务在 100% CPU 时间运行，低优先级任务永远得不到调度。

## 修复

将 `ulBitsToClearOnEntry` 改为 `SIGNAL_MOTOR_DATA_READY`，进入等待前清除处理期间积压的通知位：

```c
xTaskNotifyWait(SIGNAL_MOTOR_DATA_READY, SIGNAL_MOTOR_DATA_READY,
                &ulNotifiedValue, portMAX_DELAY)
```

这样每次唤醒只对应一个新中断，Task 处理完一帧后真正阻塞，等待下一个中断。
