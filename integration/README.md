# 集成到现有工程

Zeplod 以 **C99 源码库** 分发，不绑定构建系统。任选一种方式接入厂商 Demo / CubeMX / MCUXpresso / Keil / IAR 工程。

## 三步通用流程

1. **放入源码树** — 将本仓库作为子模块、`ThirdParty/zeplod-baremetal` 或同级目录。
2. **配置 `bm_config.h`** — 从根目录 `bm_config.h.template` 复制到应用 include 目录（须在框架头之前）。
3. **选后端** — 链 `platform/backends/` 中参考实现，或 `BACKEND external` + 自写胶水对接厂商 HAL。

## 按构建系统选择

| 场景 | 做法 |
|------|------|
| **CMake**（含 CubeMX CMake、MCUXpresso） | [`cmake/zeplod.cmake`](../cmake/zeplod.cmake) — 见 [`cmake-snippet/`](cmake-snippet/) |
| **Keil µVision** | `python tools/list_sources.py --profile event --backend register_stm32g4 --format keil` |
| **IAR EWARM** | 同上，`--format iar` |
| **Makefile / 自有 IDE** | `--format makefile` 或 `--format json` |
| **仅事件队列（8 位）** | 只 `#include "bm_ultra.h"`，不链 `src/` |

## CMake 最简示例

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/zeplod-baremetal/cmake/zeplod.cmake)

zeplod_configure(
    ROOT    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/zeplod-baremetal
    PROFILE event
    BACKEND register_stm32g4
    CONFIG  ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/bm_config.h
)

# 你的 CubeMX / SDK 目标（已含 startup、HAL、main.c）
zeplod_link(${CMAKE_PROJECT_NAME})
```

`PROFILE` 预设：`minimal` | `event` | `servo` | `full`（见 `cmake/zeplod_profiles.cmake`）。

## 对接厂商 HAL（STM32 / NXP / 其他）

参考后端是 **起点**，生产环境推荐 `BACKEND external`：

1. 保留厂商工程里的 `HAL_UART_*`、`TIM_*`、中断向量等。
2. 新增一个 `bm_drv_cubemx_glue.c`（命名随意），实现 `bm_drv_*_api` 函数表，内部调用厂商 API。
3. `zeplod_configure(BACKEND external)`，把胶水文件加入同一可执行目标。

寄存器参考实现位于 `platform/backends/register_*`，可复制后改为调用 `HAL_*`。

## 生成 IDE 文件列表

```bash
# Keil：事件 + 模块 + 看门狗 + STM32G4 参考后端
python tools/list_sources.py --profile event --backend register_stm32g4 \
    --format keil --root-macro ZEPLOD_ROOT > zeplod_sources_keil.txt

# 头文件搜索路径
python tools/list_sources.py --profile event --backend register_stm32g4 \
    --list-includes --format keil --root-macro ZEPLOD_ROOT > zeplod_includes_keil.txt
```

在 Keil / IAR 中定义宏 `ZEPLOD_ROOT` 指向框架根目录，将列表中的文件加入工程组 **Zeplod**。

## 厂商专项说明

| 厂商 | 说明 |
|------|------|
| STM32 CubeMX | [`stm32-cubemx.md`](stm32-cubemx.md) |
| NXP MCUXpresso | [`nxp-mcuxpresso.md`](nxp-mcuxpresso.md) |
| 通用 Keil / IAR | [`../docs/porting/`](../docs/porting/) |
