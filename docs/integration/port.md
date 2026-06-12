# Port（平台移植层）

类比 FreeRTOS `portable/<compiler>/<mcu>/port.c`。

模板：[`portable/template/bm_port.c`](../../portable/template/bm_port.c)

## 必须实现

| 符号 | 用途 |
|------|------|
| `bm_drv_critical_api` | 临界区 |
| `bm_drv_memory_api` | 内存屏障 |

## 按功能选用

| 符号 | 组件 |
|------|------|
| `bm_drv_timer_api` | 看门狗、HRT |
| `bm_drv_uart_api` | 日志、Shell |
| PWM/ADC 等 | 混合域 |

参考：`portable/register_stm32g4/` 或对接 Cube `HAL_*`。
