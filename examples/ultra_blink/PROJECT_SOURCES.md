# ultra_blink — Keil / IAR / GCC 源文件清单

## GCC / QEMU

### 源文件
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口 |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件 |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |

### 头文件路径
- `../../include`
- `.`

### 链接器
- `../../hal_reference/qemu_cortex_m0/linker.ld`

## Keil / IAR 真机移植

- 源文件同上（替换 `startup_qemu_cm0.s` 为目标芯片启动文件）。
- 链接器使用 Keil `.sct` 或 IAR `.icf`，参考 `docs/porting/`。
