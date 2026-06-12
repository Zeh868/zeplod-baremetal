# 08 HAL 移植指南

> **本文职责**：HAL 契约、driver API 分层、参考平台与混合域外设移植要点。  
> **不负责**：Keil/IAR 加源文件 → [18](18-Keil集成.md)、[19](19-IAR集成.md)；挂库总览 → [13](13-集成到现有工程.md)。

应用与 `Source/` 只依赖 `include/bm_hal_*.h`；厂商 Port 在 `portable/`（量产用 `template/bm_port.c` 接 SDK）。混合域 **bind 如何接到向量 ISR** 见 [03 §3.1](03-执行域与跨域通讯.md#31-直接-hal-绑定不用-bm_ctrl_inst)。

## 三层结构（driver API）

```text
include/bm_hal_*.h          应用契约（稳定 API）
include/drv/bm_drv_*.h    驱动 API 表（Zephyr 同构 vtable）
Source/hal/            分发层（契约 → driver API）
portable/              Port 实现（模板 + 参考后端）
```

| 层级 | 职责 |
|------|------|
| 契约 `bm_hal_*` | 应用可见 API，不暴露 SDK 类型 |
| 驱动 API `bm_drv_*` | 子系统函数表 + 设备 `{api, config}` |
| 分发层 | 未链接后端时返回 `BM_ERR_NOT_INIT` |
| 后端 | 实现 `bm_drv_*_api`，对接 Cube / IDF / 寄存器 |

## 参考平台

| 后端目录 | 定位 |
|----------|------|
| `portable/native_sim` | PC 测试与示例 |
| `portable/register_stm32g4` | STM32G4 寄存器参考 |
| `portable/register_esp32wroom32e` | ESP32 寄存器参考 |
| `portable/register_ch32v003` | CH32V003 桩 |
| `portable/qemu_cortex_m0` | QEMU 冒烟 |
| `portable/boot/qemu_cortex_m0/` | QEMU 启动、`crt0`、链接脚本 |
| `portable/template/bm_port.c` | 量产 Port 模板 |

## CMake 链接

```cmake
set(BM_BACKEND "native_sim" CACHE STRING "")
add_subdirectory(zeplod-baremetal)
target_link_libraries(my_app PRIVATE bm_framework bm_hal_native)    # PC
target_link_libraries(my_app PRIVATE bm_framework bm_hal_stm32g4)   # G4 板
target_link_libraries(my_app PRIVATE bm_framework bm_hal_esp32wroom32e)
```

**务必链接后端库**（`bm_hal_<platform>` 为 `bm_backend_*` 别名）。仅链 `bm_hal` 分发层时：

- `bm_hal_adc_bind_complete` 等返回 `BM_ERR_NOT_INIT`
- 无向量 ISR，绑定了也不会进电流环

见 [11-安全与可靠性](11-安全与可靠性.md) fail-open 说明。

## 混合域外设要点

| API | 行为 |
|-----|------|
| `bind_complete` / `bind_update` | 保存 `bm_hal_hrt_binding_t` 到 HAL 静态变量；ISR 调 `callback(context)` |
| `binding == NULL` | 先关中断源，再清 callback |
| 绑定 | **不得**隐式使能 PWM 输出或启动 ADC 序列 |

PWM/ADC/COMP/Encoder 契约头文件：`bm_hal_pwm.h`、`bm_hal_adc.h` 等。

移植检查：实现本应用用到的 API → `native_sim` 单测 → 真机时序。

## 移植（Port）与集成的关系

| 概念 | 内容 | 位置 |
|------|------|------|
| **库** | 事件、HAL 分发层、混合域 | `Source/`、`include/` |
| **Port** | `bm_drv_*_api` 实现 | [`portable/template/bm_port.c`](../portable/template/bm_port.c)，[14-Port移植层](14-Port移植层.md) |
| **集成** | 库怎么进 Keil/IAR/CMake | [13-集成到现有工程](13-集成到现有工程.md) |

本文描述 **Port 要实现什么**；挂库步骤见 [13](13-集成到现有工程.md)。

## 相关集成文档

| 文档 | 内容 |
|------|------|
| [13-集成到现有工程](13-集成到现有工程.md) | 两步挂库总览 |
| [16-STM32-CubeMX集成](16-STM32-CubeMX集成.md) | Cube 工程 |
| [17-NXP-MCUXpresso集成](17-NXP-MCUXpresso集成.md) | MCUX 工程 |
| [18-Keil集成](18-Keil集成.md) / [19-IAR集成](19-IAR集成.md) | IDE 手工集成 |
