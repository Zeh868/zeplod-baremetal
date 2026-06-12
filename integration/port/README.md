# Port（平台移植层）

类比 **FreeRTOS** 的 `portable/<compiler>/<mcu>/port.c`：  
Zeplod **库本体**不绑定 MCU；**Port** 由你在应用工程里实现，把 `bm_drv_*_api` 接到厂商 HAL/SDK。

## 必须实现

| 符号 | 用途 |
|------|------|
| `bm_drv_critical_api` | 全局临界区 |
| `bm_drv_memory_api` | 快照等内存屏障 |

## 按功能选用

| 符号 | 组件 |
|------|------|
| `bm_drv_timer_api` | 看门狗、HRT、Ticker |
| `bm_drv_uart_api` | 日志、Shell |
| `bm_drv_wdg_api` | 看门狗 |
| PWM/ADC/COMP/Encoder 设备驱动 | 混合域控制 |

## 起步

1. 复制 [`bm_port.c`](bm_port.c) 到 `Core/Src/`（或厂商 Demo 的 `board/`）。
2. 按注释对接 `HAL_TIM_*`、`HAL_UART_*` 等。
3. 在定时器 ISR 中调用 `bm_port_timer_isr()`（或直接在 ISR 里调 tick 回调）。

完整参考：`platform/backends/register_stm32g4/`（寄存器版）或改为调用 Cube HAL。

## 不要

- 不要把 `native_sim` / `qemu` 后端链进真机。
- 不要把 Port 编进预编译库再换 MCU（Port 始终跟应用工程走）。
