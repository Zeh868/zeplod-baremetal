# Zeplod 库集成指南

Zeplod 是 **C99 库**（类似 FreeRTOS），与厂商工程的关系分两层，顺序不能颠倒：

```text
① 移植（Port）     实现 bm_drv_*_api，对接 Cube / SDK / 寄存器
② 库集成           源码加入工程，或链接预编译 .a / .lib
```

```text
┌─────────────────────────────────────────┐
│  你的应用（main、业务、厂商 startup/HAL）   │
├─────────────────────────────────────────┤
│  Port（integration/port/bm_port.c）       │  ← 你维护，随 MCU 变
├─────────────────────────────────────────┤
│  Zeplod 库（src/ + include/）            │  ← 源码或静态库
├─────────────────────────────────────────┤
│  bm_config.h（类似 FreeRTOSConfig.h）    │
└─────────────────────────────────────────┘
```

| FreeRTOS | Zeplod |
|----------|--------|
| `FreeRTOS/Source/*.c` | `src/core/`、`src/hal/`、`src/hybrid/`（按 PROFILE 裁剪） |
| `FreeRTOSConfig.h` | `bm_config.h`（放在应用 include 最前） |
| `portable/.../port.c` | **`integration/port/bm_port.c`**（你复制并改） |
| 源码集成 / `libfreertos.a` | **方式 A 源码** / **方式 B 静态库** |

---

## 第一步：移植（Port）

1. 复制 [`port/bm_port.c`](port/bm_port.c) 到应用工程（如 `Core/Src/`）。
2. 实现 `bm_drv_critical_api`、`bm_drv_memory_api`（**必须**）。
3. 按用到的组件实现 timer / uart / wdg / PWM / ADC 等（见 [`port/README.md`](port/README.md)）。
4. 在厂商定时器 ISR 里调用 tick 回调（见 `bm_port_timer_isr`）。

HAL 契约与 driver API 细节：[docs/08-HAL移植指南.md](../docs/08-HAL移植指南.md)  
寄存器参考模板：`platform/backends/register_<mcu>/`

---

## 第二步：库集成

### 方式 A — 源码集成（推荐，与 FreeRTOS 相同）

把库 `.c` 直接加入 Keil / IAR / Makefile / CMake 工程，**不**包含 Port（Port 在第一步已单独加入）。

```bash
# 库本体源文件（不含 Port）
python tools/list_sources.py --profile event --format keil

# 头文件路径
python tools/list_sources.py --profile event --list-includes --format keil
```

在 IDE 中定义 `ZEPLOD_ROOT`，将输出加入分组 **Zeplod**；`bm_config.h` 目录放在 include 路径 **最前**。

| 构建系统 | 说明 |
|----------|------|
| Keil / IAR | 上列命令 + [`docs/porting/`](../docs/porting/) |
| Makefile | `--format makefile` |
| CMake 子目录 | 见下方「方式 A′」 |

**方式 A′ — CMake 子工程（仍是源码，只是自动选文件）**

```cmake
include(ThirdParty/zeplod-baremetal/cmake/zeplod.cmake)
zeplod_configure(ROOT ... PROFILE event BACKEND external CONFIG Core/Inc/bm_config.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE Core/Src/bm_port.c)
zeplod_link(${CMAKE_PROJECT_NAME})
```

`BACKEND external` 表示库不带平台代码，Port 由你的 `bm_port.c` 提供符号。

---

### 方式 B — 静态库集成

先为 **目标 CPU + PROFILE** 编出库，再在厂商工程里链接 `.a` / `.lib`，**Port 仍用源码编进应用**。

```bash
cmake -B build -S integration/static-lib \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
    -DZEPLOD_PROFILE=event
cmake --build build
```

产物：`libbm_core.a`、`libbm_hal.a`、`libbm_module.a` 等（见 [`static-lib/`](static-lib/)）。  
拷贝 `include/` 与这些库到厂商工程，并 **始终** 编译 `bm_port.c`。

| IDE | 做法 |
|-----|------|
| Keil | Project → Add Library + Add `bm_port.c` + Include 路径 |
| IAR | 同上 |
| CMake | `target_link_libraries(app PRIVATE zeplod::libraries)` + `bm_port.c` |

库编译时 `BM_DRV_HAS_BACKEND=1`，最终链接时由 Port 解析 `bm_drv_*_api` 符号。

---

## PROFILE（库裁剪）

| PROFILE | 内容 |
|---------|------|
| `minimal` | 事件、内存池、日志 |
| `event` | + 模块、看门狗（多数业务） |
| `servo` | + HRT、ticker、ctrl_inst |
| `full` | event + servo 全开 |

源码集成与静态库使用 **同一 PROFILE**。

---

## 厂商工程

| 场景 | 文档 |
|------|------|
| STM32 CubeMX | [`stm32-cubemx.md`](stm32-cubemx.md) |
| NXP MCUXpresso | [`nxp-mcuxpresso.md`](nxp-mcuxpresso.md) |

---

## 检查清单

- [ ] `bm_config.h` 在应用 include 路径首位
- [ ] Port（`bm_port.c`）在应用工程编译，不在预编译库里
- [ ] 库源或静态库 PROFILE 与配置一致
- [ ] 未链入 `native_sim` / `qemu` 后端
- [ ] 定时器 ISR 已接 tick / HRT 回调
