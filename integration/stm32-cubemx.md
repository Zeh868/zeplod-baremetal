# STM32 CubeMX 集成

CubeMX 生成 `Core/`、`Drivers/`、`Middlewares/` 后，Zeplod 只追加框架层，**不替换** startup 与 `stm32*_hal_*`。

## 推荐目录

```text
MyProject/
├── Core/
│   └── Inc/bm_config.h          ← 从 zeplod bm_config.h.template 复制
├── Drivers/ ...                   ← Cube 生成，保持不动
├── ThirdParty/zeplod-baremetal/   ← 子模块或拷贝
└── CMakeLists.txt               ← Cube CMake 或手写
```

## CMake（CubeMX 已导出 CMake）

在工程 `CMakeLists.txt` 末尾加入 [`cmake-snippet/CMakeLists.append.txt`](cmake-snippet/CMakeLists.append.txt) 内容，改 `BACKEND` 与 `CONFIG` 路径。

- 快速验证： `BACKEND register_stm32g4`（寄存器参考，与具体 G0/G4 系列需核对寄存器）
- 量产： `BACKEND external` + `Core/Src/bm_drv_cubemx_glue.c` 调用 `HAL_TIM_*`、`HAL_UART_*`

## Keil / IAR（Cube 生成 MDK / EWARM）

1. 在工程选项中增加 Include：`$(ZEPLOD_ROOT)/include/bm/common` 等（用 `list_sources.py --list-includes` 生成完整列表）。
2. 新建分组 **Zeplod**，加入 `list_sources.py` 输出的 `.c`。
3. 定义宏 `ZEPLOD_ROOT=..\..\ThirdParty\zeplod-baremetal`（按实际路径）。
4. 在 `main.c` 或独立 `app_main.c` 中调用 `bm_event_*`；`main` 仍由 Cube 的 `main.c` 进入。

## 胶水层要点

在 `bm_drv_cubemx_glue.c` 中实现（可复制 `platform/backends/register_stm32g4/bm_drv_singleton_stm32g4.c` 改签名）：

- `bm_drv_critical_api` — 通常 `__disable_irq` / `__enable_irq` 或 `HAL_NVIC_*`
- `bm_drv_timer_api` — 对接用于 HRT 的 TIM（在 `HAL_TIM_PeriodElapsedCallback` 里调 `bm_hrt` 绑定回调）
- `bm_drv_uart_api` — 对接 `HAL_UART_Transmit` 或 ring buffer

混合域 ISR 接线见 [03 §3.1](../docs/03-执行域与跨域通讯.md#31-直接-hal-绑定不用-bm_ctrl_inst)。

## 不要做的事

- 不要把 `platform/backends/native_sim` 或 `qemu_cortex_m0` 链进真机工程。
- 不要用 QEMU 的 `platform/boot/` 链接脚本替换 Keil scatter / IAR icf。
