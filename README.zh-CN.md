# Zeplod Baremetal

面向机电控制的资源可裁剪裸机事件驱动框架。

Zeplod Baremetal 定位于电机驱动、数字电源、电池管理系统（BMS）以及机器人末端节点等场景——在这些场景下，RTOS 过于沉重或根本不可用。它为从 8 位微控制器到中等规模 ARM Cortex-M 提供统一的编程模型，并在硬件资源充裕时，提供清晰的向 [Zeplod on Zephyr](https://github.com/zeplod/zeplod) 的迁移路径。

---

## 亮点

- **资源分级可裁剪**：从 `<8 KB Flash / <1 KB RAM`（头文件库 `bm-ultra`）到 `~12 KB Flash / ~4 KB RAM`（完整混合域控制）。
- **零堆内存设计**：所有内存在编译期或早期 `init()` 中静态分配。不使用 `malloc` / `free`。
- **混合域执行**：硬实时（HRT）控制回路与软实时（SRT）事件驱动业务逻辑严格隔离。
- **多实例控制**：同时运行多个独立控制算法（多轴伺服、多电芯 BMS），支持显式资源声明与冲突检测。
- **同步域**：对多路 PWM/ADC 进行相位锁定，适用于交错电源拓扑或同步机器人关节。
- **跨域数据交换**：无锁三缓冲快照（`bm_snapshot`），实现 HRT 到 SRT 的安全数据传递。
- **故障安全启动**：`bm_ctrl_init_all()` 在执行前先校验所有实例、资源和绑定关系；任何失败都会触发有序回滚并进入安全停机状态。

---

## 架构

```text
┌─────────────────────────────────────────────────────────────────────┐
│  应用层                                                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐ │
│  │  bm_module  │  │ bm_channel  │  │  bm_shell   │  │  bm_wdg   │ │
│  │ (模块生命周期) │  │  (SPSC通道)  │  │  (命令行)   │  │ (看门狗)   │ │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └─────┬─────┘ │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                        bm_core                                  │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────────┐  │ │
│  │  │ bm_event │  │bm_mempool│  │bm_critical│  │   bm_types     │  │ │
│  │  │(发布/订阅) │  │ (内存池)  │  │ (临界区)  │  │  (错误码体系)   │  │ │
│  │  └──────────┘  └──────────┘  └──────────┘  └────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │  混合域（可选）                                                    │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │ │
│  │  │   bm_hrt    │  │  bm_ticker  │  │      bm_ctrl_inst       │ │ │
│  │  │(定时调度 HRT)│  │ (SRT 周期任务)│  │ (多实例控制 + 资源声明)   │ │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘ │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │ │
│  │  │bm_snapshot  │  │   bm_sync   │  │    HAL 接口契约          │ │ │
│  │  │(跨域邮箱)   │  │ (相位同步)   │  │  (PWM/ADC/比较器/编码器)  │ │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
```

### 执行域

| 执行域 | 触发方式 | 典型频率 | 适用任务 |
|--------|---------|---------|---------|
| **Hardware HRT** | PWM / ADC / 比较器硬件中断 | 5–200 kHz | 电流环、数字电源、硬件故障保护 |
| **Scheduled HRT** | 专用定时器中断 | 100 Hz – 10 kHz | 速度环、低频采样、确定性派生环 |
| **SRT** | 主循环轮询 + 事件队列 | 无硬截止期限 | 状态机、通信、诊断、命令行 |

**核心规则**：Hardware HRT 与 Scheduled HRT 绝不访问 SRT 的事件队列或受临界区保护的数据结构。这保证了高优先级控制回路不会被业务逻辑处理延迟。

---

## 资源层级

| 层级 | Flash | RAM | 典型 MCU | 包含组件 |
|------|-------|-----|---------|---------|
| **Ultra** | < 8 KB | < 1 KB | STM8、AVR、8051 | `bm_ultra.h`（纯头文件库） |
| **Nano** | 8–32 KB | 1–4 KB | CH32V003、STM32F030 | `bm_core` + 可选 `bm_module` |
| **Lite** | 32–128 KB | 4–16 KB | STM32F103、nRF51822、ESP32-WROOM-32E | `bm_core` + `bm_module` + `bm_channel` + `bm_shell` |
| **Control** | 32–128 KB+ | 4–16 KB+ | STM32G4、STM32F3 | 上述全部 + `bm_hrt` + `bm_ctrl_inst` + `bm_sync` |

---

## 覆盖领域

Zeplod Baremetal 专为**资源受限 MCU 上的机电控制节点**而设计。以下按原生匹配度分级列出。

### ★★★★★ 原生匹配 — 核心设计目标

| 领域 | 典型应用场景 | 匹配原因 |
|------|------------|---------|
| **数字电源** | DC-DC、PFC、LLC、多相 VRM | Hardware HRT（PWM→ADC 触发）+ Scheduled HRT（电压环）+ 同步域（交错并联） |
| **电池管理（BMS）** | 电芯采样、库仑计数、均衡控制 | 多实例模型与“包采样器 + N 电芯”一一对应；比较器 Break 实现故障保护 |
| **伺服/运动控制** | 工业伺服、机器人关节、步进驱动 | 电流/速度/位置三环与 Hardware HRT + Scheduled HRT + SRT 完全对应 |
| **机器人** | 机械臂、移动机器人、人形末端执行器 | Zeplod 软件栈的组成部分；多轴同步 + 与上层 Zeplod/Zephyr 的事件 ID 对齐 |

### ★★★★☆ 高度适配 — 少量缺口（通信协议栈）

| 领域 | 典型应用场景 | 需补齐的缺口 |
|------|------------|------------|
| **汽车电子（QM 级）** | BCM、热管理、灯光、传感器节点 | 需集成 CAN/LIN/UDS 协议栈 |
| **无人机/飞行器** | 电调、云台电机、舵机驱动 | 需无感 FOC 算法和 DShot/OneShot 协议 |
| **消费电子/白色家电** | 变频空调、洗衣机、吸尘器 | 需显示/触摸和无线连接协议栈 |
| **能源/光伏/储能** | 光伏逆变器、PCS、充电桩模块 | 需 MPPT/PLL 和 OCPP/IEC 61850 协议 |
| **物联网/传感器节点** | 环境监测、智能表计、农业物联网 | 需 LoRa/BLE/Zigbee 和深度低功耗策略 |

### ★★★☆☆ 有限适配 — 可行但需额外投入

| 领域 | 典型应用场景 | 需补齐的缺口 |
|------|------------|------------|
| **工业自动化** | 远程 IO、步进驱动、温控 PID | 需 Modbus/工业以太网协议栈 |
| **医疗设备** | 输注泵、呼吸机电机、电动病床 | 需 IEC 62304 流程和安全认证（框架本身未认证） |
| **智能照明** | LED 驱动、DALI/DMX 解码器 | 需 DALI/DMX 和无线 mesh 协议 |

> **不适用场景**：商业航空（DO-178C）、高安全等级汽车（ASIL-D）、复杂 GUI/HMI 应用。

---

## 目录结构

```text
zeplod-baremetal/
├── include/              # 公共 API（扁平 #include，见 include/README.md）
│   ├── bm/common/        # types、log、config、atomic…
│   ├── bm/core/          # event、mempool、module、shell、wdg…
│   ├── bm/hybrid/        # hrt、ticker、ctrl_inst、sync、snapshot…
│   ├── bm/hal/           # bm_hal_* 应用契约
│   ├── bm/ultra/         # bm_ultra.h
│   └── drv/              # bm_drv_* 后端驱动 API
├── src/
│   ├── core/             # bm_event、bm_mempool、bm_critical、bm_wdg
│   ├── hal/              # bm_hal 分发层（契约 → driver API）
│   ├── module/           # bm_module
│   ├── channel/          # bm_channel
│   ├── shell/            # bm_shell
│   ├── hrt/              # bm_hrt、bm_ticker
│   └── ctrl/             # bm_ctrl_inst、bm_resource、bm_sync
├── platform/
│   ├── backends/         # 驱动后端（SDK / 寄存器 / 仿真）
│   └── boot/             # QEMU 启动汇编、链接脚本
├── examples/             # 渐进式示例（见下表）
├── tests/
│   ├── unit/             # Unity 单元测试（PC 本地运行）
│   └── qemu/             # QEMU 冒烟测试
├── docs/
│   ├── README.md         # 文档索引（00–11 导读）
│   ├── 00-快速开始.md … 11-安全与可靠性.md
│   ├── architecture.md   # 框架架构概述（英文）
│   ├── api/              # API 参考文档
│   └── porting/          # Keil / IAR 工具链附录（HAL 主文档见 08）
├── CMakeLists.txt        # CMake 构建（32 位主流平台）
└── Makefile              # 纯 Makefile（8 位工具链友好）
```

---

## 示例

[`examples/`](examples/) 目录包含逐步扩展的演示程序：

| 示例 | 重点 | 层级 |
|---------|-------|------|
| [`ultra_blink`](examples/ultra_blink) | 最小化的头文件版事件队列 | Ultra |
| [`core_sensor`](examples/core_sensor) | 事件、内存池与模块生命周期 | Nano |
| [`full_system`](examples/full_system) | 多模块、事件优先级、看门狗 | Lite |
| [`interrupt_demo`](examples/interrupt_demo) | SysTick、外设中断与 ISR 事件发布 | Nano |
| [`hrt_servo_stub`](examples/hrt_servo_stub) | 混合域伺服（电流 HRT + 速度 HRT + 位置 SRT） | Control |
| [`hrt_bms_coulomb`](examples/hrt_bms_coulomb) | BMS 包采样器（ADC HRT）+ 电芯库仑计数（SRT） | Control |
| [`multi_axis_sync`](examples/multi_axis_sync) | 带同步域的多实例控制 | Control |
| [`multi_channel_bms`](examples/multi_channel_bms) | 多通道 BMS 实例模型 | Control |

构建与运行说明见 [`examples/README.md`](examples/README.md)。

### 集成到已有工程（CubeMX / Keil / IAR / MCUXpresso）

```cmake
include(ThirdParty/zeplod-baremetal/cmake/zeplod.cmake)
zeplod_configure(ROOT .../zeplod-baremetal PROFILE event BACKEND external CONFIG Core/Inc/bm_config.h)
zeplod_link(${CMAKE_PROJECT_NAME})
```

非 CMake 工程用 `python tools/list_sources.py --profile event --backend register_stm32g4 --format keil` 生成文件列表。详见 [`integration/README.md`](integration/README.md) 与 [`docs/13-集成到现有工程.md`](docs/13-集成到现有工程.md)。

### 快速开始（本地模拟）

```bash
# 在 PC 上构建并运行示例（无需 QEMU 或硬件）
cmake -B build -S examples/core_sensor -DZEPLOD_BAREMETAL_DIR=../..
cmake --build build
./build/core_sensor
```

### 快速开始（QEMU Cortex-M0）

```bash
cmake -B build_qemu -S examples/interrupt_demo \
    -DZEPLOD_BAREMETAL_DIR=../.. \
    -DBOARD=qemu_cortex_m0
cmake --build build_qemu
qemu-system-arm -M microbit -kernel build_qemu/interrupt_demo.elf -nographic -serial stdio
```

---

## 测试

| 层级 | 目标 | 工具 | 运行环境 |
|-------|--------|------|-------------|
| **单元测试** | 算法、数据结构、状态机 | Unity + fff（mock） | PC（`native_sim`） |
| **QEMU 冒烟** | HAL 集成、中断上下文、启动流程 | 自定义 TAP 输出测试框架 | QEMU Cortex-M0 / RISC-V |
| **硬件在环** | 真实时序、功耗、外设 | 手动 + 自动化脚本 | 参考开发板 |

运行单元测试：

```bash
cmake -B build -DBM_BUILD_TESTS=ON
cmake --build build
cd build && ctest
```

---

## 配置

所有框架限制均可在编译期通过 `bm_config.h`（由 CMake 强制包含）或 `-D` 编译器标志进行配置：

```c
/* 核心事件系统 */
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4

/* 可选组件 */
#define BM_CONFIG_MAX_MODULES                8
#define BM_CONFIG_SHELL_BUF_SIZE            64
#define BM_CONFIG_MAX_WDG_MODULES            4

/* 混合域 */
#define BM_CONFIG_HRT_TICK_US                100
#define BM_CONFIG_HRT_MAX_SLOTS              16
#define BM_CONFIG_MAX_CTRL_INSTANCES         16
#define BM_CONFIG_MAX_RESOURCE_CLAIMS        64
```

将 `bm_config.h` 放在应用层的 include 路径中，或在 CMake 中设置 `BM_CONFIG_FILE`。

---

## 迁移路径

Zeplod Baremetal 被设计为机器人/电子系统三层架构的底层：

```text
[Ultra]  --升级芯片-->  [Core]  --需要模块化-->  [Core + Module]
                                                         |
                                                         v
                                             [Zephyr + Zeplod 完整版]
```

- **Ultra → Core**：编译期事件绑定改为运行时订阅；引入内存池；获得优先级和多订阅者能力。
- **Core → Core+Module**：分散的中断/主循环逻辑封装为带生命周期管理的模块。
- **Baremetal → Zephyr**：模块代码基本可复用；事件 API 语义对齐。`bm-channel` 必须替换为 Zeplod Data Bus。

详细指南见 [10-迁移与演进](docs/10-迁移与演进.md)。

---

## 文档

**导读（推荐从此开始）**：[`docs/README.md`](docs/README.md)

| 编号 | 文档 |
|------|------|
| 00 | [快速开始](docs/00-快速开始.md) |
| 01 | [框架概览与资源层级](docs/01-框架概览与资源层级.md) |
| 02 | [混合域与执行模型](docs/02-混合域与执行模型.md) |
| 03 | [执行域与跨域通讯](docs/03-执行域与跨域通讯.md) |
| 04 | [核心机制](docs/04-核心机制.md) |
| 05 | [构建配置与 CMake](docs/05-构建配置与CMake.md) |
| 06 | [示例与上手路径](docs/06-示例与上手路径.md) |
| 07 | [应用骨架与数据流](docs/07-应用骨架与数据流.md) |
| 08 | [HAL 移植指南](docs/08-HAL移植指南.md) |
| 09 | [测试与调试](docs/09-测试与调试.md) |
| 10 | [迁移与演进](docs/10-迁移与演进.md) |
| 11 | [安全与可靠性](docs/11-安全与可靠性.md) |
| 13 | [集成到现有工程](docs/13-集成到现有工程.md) |

专题：[integration/](integration/) · [architecture.md](docs/architecture.md) · [api/](docs/api/) · [porting/](docs/porting/)

---

## 设计原则

1. **静态优先**：优先使用编译期分配和基于宏的注册，而非运行时动态发现。
2. **显式契约**：每一条资源声明、调度槽位和跨域数据路径都必须显式声明。
3. **有界执行**：所有循环、队列和表格均有静态已知的最大尺寸；禁止无界递归。
4. **可测试性**：每个组件都可通过 `native_sim` 在 PC 上测试；QEMU 在接触硬件前提供架构级冒烟测试。
5. **故障安全**：初始化路径在提交硬件状态前先完成全部校验；失败时触发确定性回滚。

---

## 参与贡献

欢迎贡献。在添加新组件或 HAL 参考实现时，请遵循：

- 保持零堆内存原则。
- 提供 `native_sim` 实现和单元测试。
- 更新 `docs/api/` 中的相关 API 参考文档。
- 如该功能面向终端用户，请新增或扩展示例。

---

## 许可证

Zeplod Baremetal 以 [GNU Lesser General Public License v3.0](COPYING.LESSER)（LGPL-3.0）发布；其并入的 GPLv3 完整条款见 [COPYING.GPL3](COPYING.GPL3)。版权与免责声明见 [NOTICE](NOTICE)（`SPDX-License-Identifier: LGPL-3.0-or-later`）。

作为库静态链接进专有固件时，须遵守 LGPL-3.0 对**库部分**的再链接与源码/目标文件提供义务；应用层专有代码通常不受 GPL 传染性约束。裸机合规细节请以法务意见为准，运行时模型见 [docs/12-运行时与实例模型.md](docs/12-运行时与实例模型.md)。

第三方组件可能使用其他许可证，见其文件头（例如 `tests/unit/unity/` 中的 Unity 为 MIT）。

---

*Zeplod Baremetal 是 Zeplod 机器人软件栈的一部分。它不是通用 RTOS 的替代品，而是面向资源受限机电控制节点的专用框架。*
