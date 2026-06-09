# core_sensor — Keil / IAR / GCC 源文件清单

## GCC / QEMU

### 源文件
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口 |
| `../../src/core/bm_critical.c` | 临界区 |
| `../../src/core/bm_event.c` | 事件系统 |
| `../../src/core/bm_mempool.c` | 内存池 |
| `../../src/module/bm_module.c` | 模块生命周期 |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件 |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |

### 头文件路径
- `../../include`
- `.`

### 链接器
- `../../hal_reference/qemu_cortex_m0/linker.ld`

## Keil / IAR 真机移植

- 源文件同上（替换启动文件为目标芯片版本）。
- 链接器使用 Keil `.sct` 或 IAR `.icf`，参考 `docs/porting/`。
