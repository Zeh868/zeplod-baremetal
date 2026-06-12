# Zeplod Baremetal

A resource-scalable deterministic event, control, and streaming-compute framework for MCUs.

The stable implementation is a deterministic execution foundation aimed at motor drives, digital power, BMS, and robotic end-nodes, not a set of industrially mature domain algorithms. Its roadmap extends the same static-resource and deterministic-execution model to audio DSP, condition monitoring, instrumentation, sensor fusion, ultrasonic/radar front ends, and low-resolution vision. It provides a unified programming model from 8-bit microcontrollers to mid-range ARM Cortex-M, with a migration path to [Zeplod on Zephyr](https://github.com/zeplod/zeplod).

> Multi-domain capabilities are planned in the
> [deterministic streaming architecture](docs/23-多领域确定性流式架构.md);
> planned components are not presented as currently implemented features.
> Per-industry boundaries, physical pain points, and maturity evidence are
> defined in the [physical-domain depth matrix](docs/24-物理世界领域与算法深度矩阵.md).

---

## Highlights

- **Resource-scalable tiers**: From `<8 KB Flash / <1 KB RAM` (header-only `bm-ultra`) to `~12 KB Flash / ~4 KB RAM` (full hybrid-domain control).
- **Zero-heap design**: All memory is statically allocated at compile-time or during early `init()`. No `malloc` / `free`.
- **Hybrid-domain execution**: Hard real-time (HRT) control loops are isolated from soft real-time (SRT) event-driven business logic.
- **Multi-instance execution**: Run multiple control, acquisition, or estimation instances with explicit resource claims and conflict detection.
- **Synchronization domains**: Phase-lock multiple PWM/ADC instances for interleaved power stages or synchronized robotic joints.
- **Cross-domain data exchange**: Lock-free triple-buffer snapshots (`bm_snapshot`) for safe HRT-to-SRT data hand-off.
- **Failure-safe startup**: `bm_exec_init_all()` validates all instances, resources, and bindings before execution; any failure triggers orderly rollback and safe-stop.
- **Multi-domain roadmap**: Static zero-copy block streams and DMA block/frame deadlines are planned; `bm_exec` is the common execution model for control, acquisition, audio, vision, and edge DSP.

---

## Architecture

```text
┌─────────────────────────────────────────────────────────────────────┐
│  Application                                                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐ │
│  │  bm_module  │  │ bm_channel  │  │  bm_shell   │  │  bm_wdg   │ │
│  │ (lifecycle) │  │  (SPSC)     │  │   (CLI)     │  │(watchdog) │ │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └─────┬─────┘ │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                        bm_core                                  │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────────┐  │ │
│  │  │ bm_event │  │bm_mempool│  │bm_critical│  │   bm_types     │  │ │
│  │  │(pub/sub) │  │(pools)   │  │(sections) │  │ (error codes)  │  │ │
│  │  └──────────┘  └──────────┘  └──────────┘  └────────────────┘  │ │
│  └────────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │  Hybrid Domain (optional)                                       │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │ │
│  │  │   bm_hrt    │  │  bm_ticker  │  │      bm_exec       │ │ │
│  │  │(scheduled  │  │  (SRT       │  │ (generic execution      │ │ │
│  │  │  dispatch)  │  │  periodic)  │  │  + resource claims)     │ │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘ │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │ │
│  │  │bm_snapshot  │  │   bm_sync   │  │    HAL contracts        │ │ │
│  │  │(cross-domain│  │(phase sync) │  │  (PWM/ADC/COMP/ENC)     │ │ │
│  │  │  mailbox)   │  │             │  │                         │ │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
```

### Execution domains

| Domain | Trigger | Frequency | Use case |
|--------|---------|-----------|----------|
| **Hardware HRT** | PWM / ADC / COMP IRQ | 5–200 kHz | Current loop, digital power, hardware protection |
| **Scheduled HRT** | Dedicated timer ISR | 100 Hz – 10 kHz | Speed loop, low-frequency sampling, deterministic derived loops |
| **SRT** | Main-loop poll + event queue | No hard deadline | State machines, communication, diagnostics, shell |

**Key rule**: Hardware HRT and Scheduled HRT never access SRT event queues or critical-section-protected structures. This guarantees that high-priority control loops cannot be delayed by business-logic processing.

The planned **Block/Frame RT** domain is DMA or frame-ready driven and must finish
audio, FFT, acquisition, or vision work before the next block/frame arrives.

---

## Resource Tiers

| Tier | Flash | RAM | Typical MCU | Components |
|------|-------|-----|-------------|------------|
| **Ultra** | < 8 KB | < 1 KB | STM8, AVR, 8051 | `BM_CONFIG_ENABLE_ULTRA` / `bm_ultra.h` |
| **Nano** | 8–32 KB | 1–4 KB | CH32V003, STM32F030 | `zeplod.h` or `bm_lite.h` |
| **Lite** | 32–128 KB | 4–16 KB | STM32F103, nRF51822, ESP32-WROOM-32E | + channel / shell via `BM_CONFIG_ENABLE_*` |
| **Control** | 32–128 KB+ | 4–16 KB+ | STM32G4, STM32F3 | + HRT / `bm_exec` / sync (`bm_hybrid.h`) |
| **DSP (planned)** | 64–256 KB | 16–128 KB | Cortex-M4F/M7, ESP32-S3 | + algorithm / stream / pipeline |
| **Media Edge (planned)** | 128 KB+ | 64 KB+ / external RAM | Cortex-M7, PSRAM MCUs | + audio / frame / camera HAL |

---

## Target Domains

Zeplod Baremetal targets **deterministic control and streaming-compute nodes** on
resource-constrained MCUs. Current and planned coverage are distinguished below.
Coverage means the real-time MCU subsystem, not a complete PLC runtime, protocol
stack, SLAM system, medical diagnosis, safety certification, or media platform.

### ★★★★★ Native fit — core design targets

| Domain | Typical use case | Why it fits | Status |
|--------|-----------------|-------------|--------|
| **Digital power** | DC-DC, PFC, LLC, multi-phase VRM | Hardware + scheduled HRT and synchronization | Current foundation |
| **BMS** | Cell sampling, coulomb counting, balancing | Multi-instance sampling, snapshots, protection | Current foundation |
| **Servo / motion** | Servos, robot joints, steppers | Multi-rate control loops and synchronization | Current foundation |
| **Robotics** | Manipulators, mobile robots, end-effectors | Multi-axis instances and event coordination | Current foundation |
| **Acoustic/condition monitoring** | Noise, bearing, structural monitoring | DMA blocks, FFT/envelope/features | R1-R3 planned |
| **Instrumentation/metering** | Acquisition, power quality, recorders | Synchronous blocks and spectral/statistical DSP | R1-R3 planned |
| **Sensor fusion/perception** | IMU/AHRS, ultrasonic, low-rate radar | Timestamps, fusion, correlation/FFT | R2-R3 planned |
| **Mobile/work machinery low level** | AGV, agriculture, hydraulics, conveyors, 3D printers | Motion/process loops, fusion, interlocks | Component roadmap |
| **Process/building nodes** | HVAC, irrigation, door drives, semiconductor auxiliaries | Multi-rate control, sequences, derating | Component roadmap |

### ★★★★☆ Strong fit — minor gaps (protocol stacks)

| Domain | Typical use case | Gap to address |
|--------|-----------------|----------------|
| **Automotive (QM)** | BCM, thermal management, lighting, sensor nodes | Needs CAN/LIN/UDS stack integration |
| **UAV / drone** | ESC, gimbal motors, servo drives | Needs sensorless FOC and DShot/OneShot protocols |
| **Consumer / white goods** | Inverter AC, washing machine, vacuum | Needs display/touch and wireless stacks |
| **Energy / PV / ESS** | Solar inverter, PCS, EV charger module | Needs MPPT/PLL and OCPP/IEC 61850 protocols |
| **IoT / sensor nodes** | Environmental monitoring, smart metering, agriculture | Needs LoRa/BLE/Zigbee and aggressive power-management policy |
| **Embedded audio** | Intercom front ends, prompts, EQ, AGC, VAD | Needs stream/pipeline and I2S/SAI/PDM/DAC HAL |
| **Low-resolution vision** | Thresholding, morphology, marker/line detection | Needs frame streams, camera HAL, cache/external RAM |
| **Biomedical signals** | ECG/PPG/EMG preprocessing | Medical algorithms and certification remain product responsibilities |
| **Communications DSP / TinyML** | DTMF/FSK, features, anomaly detection | Protocol and inference runtimes remain external |
| **PLC/gateway/real-time Ethernet adapters** | Process images, shaping, deterministic QoS | Language runtimes and certified stacks remain external |

### ★★★☆☆ Moderate fit — viable with extra work

| Domain | Typical use case | Gap to address |
|--------|-----------------|----------------|
| **Industrial automation** | Remote IO, stepper drives, PID temperature | Needs Modbus/Industrial-Ethernet protocols |
| **Medical devices** | Infusion pump, ventilator motor, hospital bed | Needs IEC 62304 process and safety certification (framework itself is uncertified) |
| **Smart lighting** | LED driver, DALI/DMX decoder | Needs DALI/DMX and wireless mesh protocols |

> **Not a fit**: Commercial avionics, ASIL-D control, SIL4 train control,
> implantable medical devices, complete 5G baseband, high-resolution software
> video codecs, large neural models, or desktop media stacks.

---

## Directory Structure

```text
zeplod-baremetal/
├── include/              # Public API (flat; see include/README.md)
│   ├── zeplod.h          # Unified entry (#include after bm_config.h)
│   ├── bm_*.h            # Framework subsystems + HAL contracts
│   └── bm_drv_*.h        # Driver API (Port authors)
├── Source/               # Library kernel (like FreeRTOS/Source/)
│   ├── core/             # Events, mempool, module, shell, wdg…
│   ├── hal/              # HAL dispatch layer
│   └── hybrid/           # HRT, bm_exec, sync…
├── portable/             # Platform ports (template + reference backends)
├── Demo/                 # Progressive examples (like FreeRTOS/Demo/)
├── tests/
│   ├── unit/             # Unity unit tests (PC native)
│   └── qemu/             # QEMU smoke tests
├── docs/
│   ├── README.md         # Doc index (00–21)
│   ├── 00-快速开始.md … 21-测试覆盖率基线.md
│   └── api/              # API reference
├── CMakeLists.txt
└── Makefile
```

---

## Examples

The [`Demo/`](Demo/) directory contains progressive demonstrations:

| Example | Focus | Tier | Maturity |
|---------|-------|------|----------|
| [`ultra_blink`](Demo/ultra_blink) | Minimal header-only event queue | Ultra | `D0` |
| [`core_sensor`](Demo/core_sensor) | Events, mempool, and module lifecycle | Nano | `D0` |
| [`full_system`](Demo/full_system) | Multi-module, event priorities, watchdog | Lite | `D0` |
| [`interrupt_demo`](Demo/interrupt_demo) | SysTick, peripheral IRQ, and ISR event publishing | Nano | `D0` |
| [`hrt_servo_stub`](Demo/hrt_servo_stub) | Hybrid-domain servo (current HRT + speed HRT + position SRT) | Control | `D0` |
| [`hrt_bms_coulomb`](Demo/hrt_bms_coulomb) | BMS pack sampler (ADC HRT) + cell coulomb counting (SRT) | Control | `D0` |
| [`multi_axis_sync`](Demo/multi_axis_sync) | Multi-instance control with synchronization domain | Control | `D0` |
| [`multi_channel_bms`](Demo/multi_channel_bms) | Multi-channel BMS instance model | Control | `D0` |

`D0` means mechanism demonstration. It does not claim product-ready or industrially
mature servo, FOC, BMS, or synchronization algorithms. See the
[domain algorithm roadmap](docs/22-领域算法与模块化路线图.md#11-技术深度与成熟度要求).

See [`docs/06-示例与上手路径.md`](docs/06-示例与上手路径.md) for build and run instructions.

### Quick start (native simulation)

```bash
# Build and run an example on PC (no QEMU, no hardware)
cmake -B build/demo/manual/core_sensor -S Demo/core_sensor
cmake --build build
./build/core_sensor
```

### Quick start (QEMU Cortex-M0)

```bash
.\tools\demo\run_qemu.ps1 interrupt_demo
# or: cmake -B build/demo/qemu/interrupt_demo -S Demo/interrupt_demo ...
cmake --build build_qemu
qemu-system-arm -M microbit -kernel build_qemu/interrupt_demo.elf -nographic -serial stdio
```

### Integrate into an existing project

**① Port** `portable/template/bm_port.c` → wire to Cube/SDK HAL  
**② Library** add `Source/` sources or link `libbm_*.a` (see `cmake/static-lib/`)

```cmake
zeplod_configure(ROOT ... PROFILE event BACKEND external CONFIG bm_config.h)
target_sources(app PRIVATE Core/Src/bm_port.c)
zeplod_link(app)
```

```c
#include "zeplod.h"
#include "bm_hal_uart.h"   /* board HAL as needed */
```

See [`docs/13-集成到现有工程.md`](docs/13-集成到现有工程.md) and [`docs/20-头文件布局.md`](docs/20-头文件布局.md).

---

## Testing

| Layer | Target | Tool | Environment |
|-------|--------|------|-------------|
| **Unit tests** | Algorithms, data structures, state machines | Unity + fff (mocks) | PC (`native_sim`) |
| **QEMU smoke** | HAL integration, interrupt context, boot flow | Custom TAP-output runner | QEMU Cortex-M0 / RISC-V |
| **Hardware-in-loop** | Real timing, power, peripherals | Manual + automated scripts | Reference dev boards |

Run unit tests:

```bash
cmake -B build -DBM_BUILD_TESTS=ON
cmake --build build
cd build && ctest
```

---

## Configuration

All framework limits are configurable at compile-time via `bm_config.h` (force-included by CMake) or via `-D` compiler flags:

```c
/* Core event system */
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4

/* Optional components */
#define BM_CONFIG_MAX_MODULES                8
#define BM_CONFIG_SHELL_BUF_SIZE            64
#define BM_CONFIG_MAX_WDG_MODULES            4

/* Hybrid domain */
#define BM_CONFIG_HRT_TICK_US                100
#define BM_CONFIG_HRT_MAX_SLOTS              16
#define BM_CONFIG_MAX_EXEC_INSTANCES         16
#define BM_CONFIG_MAX_RESOURCE_CLAIMS        64
```

Place `bm_config.h` in your application include path (before `zeplod-baremetal/include`), or set `BM_CONFIG_FILE` in CMake.

Application entry:

```c
#include "zeplod.h"         /* exposes APIs per BM_CONFIG_ENABLE_* */
#include "bm_hal_uart.h"    /* board HAL as needed */
```

Include path: **one** directory — `zeplod-baremetal/include/`. See [docs/20-头文件布局.md](docs/20-头文件布局.md).

---

## Migration Path

Zeplod Baremetal is designed as the bottom layer of a three-tier robot/electronics stack:

```text
[Ultra]  --upgrade chip-->  [Core]  --need modularity-->  [Core + Module]
                                                       |
                                                       v
                                           [Zephyr + Zeplod full]
```

- **Ultra → Core**: Compile-time event binding becomes runtime subscription; introduce memory pools; gain priorities and multi-subscribers.
- **Core → Core+Module**: Scatter-gather logic is wrapped into modules with lifecycle management.
- **Baremetal → Zephyr**: Module code is largely reusable; event API semantics are aligned. `bm-channel` must be replaced with Zeplod Data Bus.

See [docs/10-迁移与演进.md](docs/10-迁移与演进.md) for detailed guides.

---

## Documentation

Start at [`docs/README.md`](docs/README.md) (numbered guides 00–23 in Chinese).

- [`docs/01-框架概览与资源层级.md`](docs/01-框架概览与资源层级.md) — Architecture overview
- [`docs/13-集成到现有工程.md`](docs/13-集成到现有工程.md) — Integrate into existing Cube/SDK/Keil/IAR projects
- [`docs/20-头文件布局.md`](docs/20-头文件布局.md) — `zeplod.h` entry and flat `include/` layout
- [`docs/08-HAL移植指南.md`](docs/08-HAL移植指南.md) — HAL contracts and porting
- [`docs/api/`](docs/api/) — Hybrid-domain API reference
- [`docs/06-示例与上手路径.md`](docs/06-示例与上手路径.md) — Examples and hardware porting
- [`docs/22-领域算法与模块化路线图.md`](docs/22-领域算法与模块化路线图.md) — Domain algorithms and maturity
- [`docs/23-多领域确定性流式架构.md`](docs/23-多领域确定性流式架构.md) — Multi-domain deterministic streaming architecture

---

## Design Principles

1. **Static-first**: Prefer compile-time allocation and macro-based registration over runtime discovery.
2. **Explicit contracts**: Every resource claim, scheduling slot, and cross-domain data path is declared explicitly.
3. **Bounded execution**: All loops, queues, and tables have statically known maximum sizes; no unbounded recursion.
4. **Testability**: Every component is testable on PC via `native_sim`; QEMU provides architecture-level smoke tests before hardware is touched.
5. **Fail-safe**: Initialization paths validate everything before committing hardware state; failures trigger deterministic rollback.

---

## Contributing

Contributions are welcome. When adding new components or HAL references:

- Maintain the zero-heap policy.
- Provide `native_sim` implementations and unit tests.
- Update the relevant API reference in `docs/api/`.
- Add or extend an example if the feature is user-facing.

---

## License

Zeplod Baremetal is free software licensed under the [GNU Lesser General Public License v3.0](COPYING.LESSER) (LGPL-3.0). The incorporated GPLv3 terms are included in [COPYING.GPL3](COPYING.GPL3). See [NOTICE](NOTICE) (`SPDX-License-Identifier: LGPL-3.0-or-later`).

When you statically link this library into proprietary firmware, LGPL-3.0 obligations apply to the **library portion** (relinking and Corresponding Source for the library). Application code is generally not subject to GPL copyleft. Consult legal counsel for bare-metal compliance; see [docs/12-运行时与实例模型.md](docs/12-运行时与实例模型.md).

Third-party files may use other licenses; see their headers (e.g. Unity test framework: MIT).

---

*Zeplod Baremetal is part of the Zeplod robotics/electronics stack. It is not a
general-purpose RTOS or media stack; it targets deterministic control and bounded
streaming computation on resource-constrained MCUs.*
