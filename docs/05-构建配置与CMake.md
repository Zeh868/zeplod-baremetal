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
| `bm_hal_native` / `bm_hal_stm32g4` / … | 平台强符号覆盖 |
| `bm_framework` | 已启用组件的聚合接口库 |

应用应**只链接用到的目标**；需要全开时用 `bm_framework`。

## 应用工程示例

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app C)

set(ZEPLOD_BAREMETAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../zeplod-baremetal")
add_subdirectory(${ZEPLOD_BAREMETAL_DIR} zeplod)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE bm_hal_native bm_framework)
```

示例目录通过 `-DZEPLOD_BAREMETAL_DIR=../..` 指向框架根。

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
#define BM_CONFIG_HRT_DEADLINE_MISS_LOG      0   /* 1=弱钩子默认打错误日志 */
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
