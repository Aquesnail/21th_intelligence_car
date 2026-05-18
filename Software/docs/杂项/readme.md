pdata,code
![alt text](image.png)
![alt text](image-1.png)
![alt text](image-2.png)
![alt text](image-3.png)
![alt text](image-4.png)
![alt text](image-5.png)
![alt text](image-6.png)
![alt text](image-7.png)
![alt text](image-8.png)
我就不知道printf打印的时候会把所有指针转成默认指针类型，tnnd我说看文档怎么都和我打印的东西对不上，最后还得听它语速慢没字幕的课。。。

![alt text](image-9.png)
near和edata是同一个东西

引用edata/near  xdata区域的指针都是16位的
edata/near -> far指针，0x00基地址+16位偏移量
xdata -> far指针，0x01基地址+16位偏移量

far -> xdata指针 只保留最后16位偏移量
far -> edata指针 只保留最后16位偏移量
uint32_t GetRunTimeCounterValue(void)
{
    uint16_t u16High;
    uint16_t u16Low;

    do {
        u16High = g_u16OverflowCount;
        u16Low  = ((uint16_t)T11H << 8) | T11L;
    } while (u16High != g_u16OverflowCount);

    return ((uint32_t)u16High << 16) | u16Low;
}这么写不用关中断，还几乎没有什么性能损失，太nb了

---

## 上述 do...while 写法的致命缺陷 & CPU 统计 >100% 问题

### 现象

打开 main_control 任务后（P2.0 500Hz 中断），vTaskGetRunTimeStats 输出的各任务 CPU 占用率加起来超过 100%。

### 根因

do...while 只能检测 **"读取期间发生了溢出中断（g_u16OverflowCount 变化）"**，但检测不到 **"硬件已经溢出，ISR 还没来得及执行"**：

```
定时器: 65535 → 0 (溢出，T11IF=1)
g_u16OverflowCount: 还是旧值 N (ISR 被其他中断/P2 ISR 阻塞)
do...while 读到: u16High=N, u16Low=100
g_u16OverflowCount 还是 N → while 条件为假 → 退出
返回: (N << 16) | 100  ← 比正确值少了 65536，计数器倒退了！
```

FreeRTOS 在 tasks.c 上下文切换时发现 `ulTotalRunTime < ulTaskSwitchedInTime`，静默丢弃这段时间。500Hz P2 中断导致上下文切换极频繁，大量采样命中这个窗口，累积误差使得各任务 ulRunTimeCounter 之和远超总耗时 ulTotalTime，百分比超过 100%。

### 两个修复

**1. timer.c: TIMER11 ISR 必须软件清除中断标志**

```c
void TIMER11_ISR(void) interrupt TMR11_VECTOR
{
    TIMER11_ClearFlag();  // STC32G 的 T11IF 需要软件清除！
    g_u16OverflowCount++;
}
```

对比同文件的 TIMER9 ISR 就有 `TIMER9_ClearFlag()`，TIMER11 之前漏了。

**2. cpu_status.c: GetRunTimeCounterValue() 回退到关中断 + 硬件标志补偿**

```c
uint32_t GetRunTimeCounterValue(void)
{
    uint16_t u16High, u16Low;
    unsigned char ucState;

    ucState = EA;
    EA = 0;                               // 关中断保证原子性

    u16High = g_u16OverflowCount;
    u16Low  = ((uint16_t)T11H << 8) | T11L;

    // 硬件溢出但 ISR 未来得及执行 → 手动补偿
    if (TIMER11_CheckFlag() && u16Low < 0x8000u) {
        u16High++;
    }

    EA = ucState;

    return ((uint32_t)u16High << 16) | u16Low;
}
```

补偿条件：
- `TIMER11_CheckFlag()` = 1：硬件说"我溢出了"
- `u16Low < 0x8000u`：定时器当前值在低半区（刚溢出从 0 开始，值一定很小），确认真的是近期溢出

补偿只改局部变量 u16High，不改全局 g_u16OverflowCount。函数返回后 ISR 执行，全局 +1，两条路径收敛到同一个值，不会 double-count。

### 总结

do...while 看似巧妙，但在 ISR 延迟执行时会读到倒退的计数器值。对于单核 MCU 上硬件溢出和 ISR 执行之间存在时间窗口的场景，关中断 + 读硬件标志补偿是唯一正确的方案。