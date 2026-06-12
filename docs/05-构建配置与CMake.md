# 05 构建配置与 CMake

## 顶层 CMake 选项

在框架根目录 `CMakeLists.txt` 中通过 `option` 裁剪组件：

| 选项 | 默认 | 说明 |
|------|------|------|
| `BM_BUILD_TESTS` | OFF | 构建 `tests/unit` |
| `BM_ENABLE_MODULE` | ON | `bm_module` |
| `BM_ENABLE_CHANNEL` | OFF | `bm_channel` |
| `BM_ENABLE_SHELL` | OFF | `bm_shell` |
| `BM_ENABLE_WDG` | ON | `bm_wdg` |
| `BM_ENABLE_HRT` | OFF | `bm_hrt` |
| `BM_ENABLE_TICKER` | OFF | `bm_ticker` |
| `BM_ENABLE_CTRL_INST` | OFF | `bm_ctrl_inst`、`bm_resource`（依赖 HRT） |
| `BM_ENABLE_SYNC` | OFF | `bm_sync`（依赖 CTRL_INST） |
| `BM_SYNC_HAL_NATIVE` | — | 测试用 native 同步 HAL |

依赖链：

```text
BM_ENABLE_SYNC → BM_ENABLE_CTRL_INST → BM_ENABLE_HRT
```

## CMake 目标

| 目标 | 内容 |
|------|------|
| `bm_core` | 事件、内存池、临界区 |
| `bm_module` / `bm_channel` / `bm_shell` / `bm_wdg` | 可选组件 |
| `bm_hrt` / `bm_ticker` / `bm_ctrl_inst` / `bm_resource` / `bm_sync` | 混合域 |
| `bm_hal` | 弱符号默认桩 |
| `bm_hal_native` / `bm_hal_stm32g4` / `bm_hal_esp32wroom32e` / … | 平台强符号覆盖 |
| `bm_framework` | 已启用组件的聚合接口库 |

应用应**只链接用到的目标**；需要全开时用 `bm_framework`。

## 应用工程集成（推荐）

使用单入口 `cmake/zeplod.cmake`（见 [13-集成到现有工程](13-集成到现有工程.md)）：

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/zeplod-baremetal/cmake/zeplod.cmake)

zeplod_configure(
    ROOT    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/zeplod-baremetal
    PROFILE event
    BACKEND register_stm32g4
    CONFIG  ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h
)

add_executable(my_app main.c)   # 或与 CubeMX 目标合并
zeplod_link(my_app)
```

`PROFILE` 预设裁剪组件；`BACKEND external` 表示胶水层在应用工程内对接厂商 HAL。

## 底层 add_subdirectory（高级）

仍可直接 `add_subdirectory(zeplod-baremetal)` 并手动设置 `BM_ENABLE_*`、`BM_BACKEND`。框架内示例与单元测试采用此方式。

## `bm_config.h`

所有上限在编译期固定。在应用 include 路径放置 `bm_config.h`，或通过 CMake 设置 `BM_CONFIG_FILE`。

常用宏：

```c
/* 事件 */
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_EVENT_PRIORITY_BURST_MAX   8

/* 模块 / Shell / 看门狗 */
#define BM_CONFIG_MAX_MODULES                8
#define BM_CONFIG_SHELL_BUF_SIZE            64
#define BM_CONFIG_SHELL_MAX_ARGS             4
#define BM_CONFIG_SHELL_MAX_NAME_LEN         16
#define BM_CONFIG_WDG_MODULE_TIMEOUT_MS   1000

/* 混合域 */
#define BM_CONFIG_HRT_TICK_US                100
#define BM_CONFIG_HRT_MAX_SLOTS              16
#define BM_CONFIG_MAX_CTRL_INSTANCES         16
#define BM_CONFIG_MAX_RESOURCE_CLAIMS        64
#define BM_CONFIG_MAX_SYNC_MEMBERS           BM_CONFIG_MAX_CTRL_INSTANCES
#define BM_CONFIG_SYNC_MAX_PHASE_TICKS       1000000000u
```

模板见仓库根 `bm_config.h.template`。CMake 会 **force-include** 该头，保证框架与应用宏一致。

也可用编译器 `-D` 覆盖单个宏。

## Makefile 路径

8 位或极简工具链可用根目录 `Makefile`，不依赖 CMake。见 `examples/` 中部分 Makefile 示例。

## 8 位 Ultra

不链接 `src/`，仅 `#include "bm_ultra.h"`。见 `examples/ultra_blink`。

## 常见问题

| 现象 | 处理 |
|------|------|
| 链接到弱符号 HAL，外设无反应 | 链接对应 `bm_hal_<platform>` |
| `BM_ENABLE_CTRL_INST` 配置失败 | 同时打开 `BM_ENABLE_HRT` |
| MSVC 与头文件换行 | 头文件保持 LF，避免 C2143 |
