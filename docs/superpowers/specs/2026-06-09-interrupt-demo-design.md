# interrupt_demo 设计规格

## 1. 目标

创建第四个渐进示例 `interrupt_demo`，演示：
- **多个 NVIC 中断向量**：SysTick（CPU 内部）+ TIMER1（nRF51 外设）
- **ISR 中发布事件**：中断处理程序调用 `bm_event_publish_event_from_isr()`
- **纯事件驱动主循环**：`while(1) { bm_event_process(); }`，无 `delay_cycles()`
- **中断优先级**：SysTick 高于 TIMER1，展示抢占/嵌套概念

## 2. 架构

```
+-------------+      +-------------------+      +------------------+
| SysTick IRQ | ---> | SysTick_Handler   | ---> | publish EVENT_TICK|
| (10Hz)      |      | (高优先级)         |      | from ISR          |
+-------------+      +-------------------+      +------------------+
                                                        |
+-------------+      +-------------------+      +------------------+
| TIMER1 IRQ  | ---> | TIMER1_IRQHandler | ---> | publish EVENT_SAMPLE
| (2Hz)       |      | (低优先级)         |      | from ISR          |
+-------------+      +-------------------+      +------------------+
                                                        |
                                                        v
                                              +-------------------+
                                              |   Event Queue     |
                                              +-------------------+
                                                        |
                                                        v
                                              +-------------------+
                                              | main() loop       |
                                              | bm_event_process()|
                                              +-------------------+
                                                        |
                              +-------------------------+-------------------------+
                              |                         |                         |
                              v                         v                         v
                        +-------------+         +-------------+         +-------------+
                        | tick_cb     |         | sample_cb   |         | stats_cb    |
                        | 输出 [TICK] |         | 输出 [SAMPLE]|        | 输出统计    |
                        +-------------+         +-------------+         +-------------+
```

## 3. 中断源设计

### 3.1 SysTick（系统节拍）
- **频率**：100ms（10Hz）
- **优先级**：最高（Cortex-M 默认）
- **处理程序**：`SysTick_Handler`
- **动作**：
  1. `_ticks++`
  2. 如果注册了用户回调，调用之
  3. 发布 `EVENT_TICK`（from ISR）

### 3.2 TIMER1（nRF51 外设）
- **频率**：500ms（2Hz）
- **优先级**：低于 SysTick（NVIC 优先级 3）
- **处理程序**：`TIMER1_IRQHandler`
- **动作**：
  1. 清除 TIMER1 比较事件
  2. 发布 `EVENT_SAMPLE`（from ISR），携带模拟温度数据

### 3.3 回退方案
若 QEMU 不完全支持 nRF51 TIMER1，回退到 **SVC 软件中断**：
- `main()` 中每隔 500ms 执行 `__asm volatile("svc 0")`
- `SVC_Handler` 中发布 `EVENT_SAMPLE`
- 规格其余部分不变

## 4. 事件定义

| 事件类型 | 名称 | 触发源 | 数据 | 优先级 |
|---------|------|--------|------|--------|
| `EVENT_TICK` (1) | TICK | SysTick ISR | `uint32_t` tick_count | 0 (最高) |
| `EVENT_SAMPLE` (2) | SAMPLE | TIMER1 ISR | `uint16_t` temp | 1 |
| `EVENT_STATS` (3) | STATS | main 循环 | 无 | 2 |

## 5. 主循环逻辑

```c
while (1) {
    // 处理事件队列（中断会自动填充队列）
    bm_event_process(8);

    // 每处理完一轮，检查是否需要输出统计
    if (g_tick_count >= 20 && !stats_sent) {
        publish EVENT_STATS;
        stats_sent = 1;
    }

    // 终止条件：至少 2 个 SAMPLE + STATS 输出完毕
    if (g_sample_count >= 2 && stats_sent && !pass_sent) {
        uart_print("EXAMPLE_IRQ: PASS\n");
        pass_sent = 1;
    }
}
```

## 6. 回调执行顺序（LIFO）

- `EVENT_TICK`：先订阅 `stats_cb`（priority=0），后订阅 `tick_cb`（priority=1）
  - 执行顺序：`tick_cb` 先（LIFO），`stats_cb` 后
- `EVENT_SAMPLE`：订阅 `sample_cb`

## 7. 框架改动

### 7.1 `startup_qemu_cm0.s` — 扩展向量表

当前向量表只有 4 个条目（stack + reset + NMI + HardFault）。需扩展至包含所有 Cortex-M0 系统异常 + nRF51 IRQ0~IRQ31，全部指向 `Default_Handler`。这样任何示例都可以安全启用中断，不会跳转到随机地址。

扩展后的向量表：
```
0x00: _estack
0x04: Reset_Handler
0x08: Default_Handler  (NMI)
0x0C: Default_Handler  (HardFault)
0x10~0x1B: 0 (Reserved)
0x1C: Default_Handler  (SVC)
0x20~0x23: 0 (Reserved)
0x24: Default_Handler  (PendSV)
0x28: Default_Handler  (SysTick)
0x2C~0xA8: Default_Handler  (IRQ0~IRQ31)
```

### 7.2 `bm_hal_timer_qemu.c` — 增加 TIMER1 支持

新增 TIMER1 初始化函数：
```c
void bm_hal_timer1_init(uint32_t period_ms);
void bm_hal_timer1_set_callback(void (*cb)(void));
```

或使用内联方式直接在 `main.c` 中配置寄存器（减少框架改动）。

**本设计选择**：为保持示例自包含，TIMER1 配置直接写在 `interrupt_demo/main.c` 中（寄存器级操作），不修改框架 HAL。这样示例更清晰地展示了如何在裸机环境下直接配置外设中断。

## 8. 文件结构

```
examples/
├── interrupt_demo/
│   ├── bm_config.h
│   ├── main.c
│   ├── CMakeLists.txt
│   ├── Makefile
│   └── PROJECT_SOURCES.md
```

## 9. 脚本更新

### 9.1 `run_all.ps1` / `run_all.sh` / `run_all.bat`
- 在 `EXAMPLES` 数组中追加 `interrupt_demo`

### 9.2 新增 `run_single.ps1`
- 用法：`.un_single.ps1 <example_name>`
- 功能：
  1. 切换到指定示例目录
  2. `cmake -B build ... && cmake --build build`
  3. 在前台运行 QEMU：`qemu-system-arm ... --semihosting -nographic -serial stdio`
  4. 实时输出到控制台（不截断、不后台、不超时杀死）
- 支持交互：用户可以按 `Ctrl+A X` 退出 QEMU

### 9.3 新增 `run_single.sh`
- Bash 版本，功能同上

### 9.4 新增 `run_single.bat`
- 调用 `run_single.ps1` 的包装

## 10. 验证标准

### 10.1 构建
- CMake + Makefile 均零警告通过

### 10.2 QEMU 输出（约 3 秒）
```
IRQ Demo: interrupt_demo
[TICK] count=1
[TICK] count=2
[TICK] count=3
[TICK] count=4
[TICK] count=5
[SAMPLE] temp=25
[TICK] count=6
[TICK] count=7
[TICK] count=8
[TICK] count=9
[TICK] count=10
[SAMPLE] temp=26
[STATS] ticks=20, samples=2
EXAMPLE_IRQ: PASS
```

### 10.3 自动化测试
- `run_all.ps1` / `run_all.sh` / `run_all.bat` 报告 `interrupt_demo ... PASS`
- `run_single.ps1 interrupt_demo` 能在前台运行并显示完整输出

## 11. 关键约束

- **零堆分配**：ISR 中只使用 `bm_event_publish_event_from_isr()`，不调用 `malloc`
- **中断安全**：使用 `bm_event_publish_event_from_isr()` 而非普通 `publish`
- **确定性**：SysTick 和 TIMER1 周期固定，输出可预测
- **向后兼容**：`startup_qemu_cm0.s` 扩展后，现有三个示例行为不变
