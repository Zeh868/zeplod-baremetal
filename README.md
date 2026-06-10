# Zeplod Baremetal

A resource-scalable bare-metal event-driven framework for MCU-based electromechanical control.

Zeplod Baremetal targets motor drives, digital power, battery management systems (BMS), and robotic end-nodes where an RTOS is too heavy or simply unavailable. It provides a unified programming model across 8-bit microcontrollers up to mid-range ARM Cortex-M, with a clear migration path to [Zeplod on Zephyr](https://github.com/zeplod/zeplod) when hardware resources allow.

---

## Highlights

- **Resource-scalable tiers**: From `<8 KB Flash / <1 KB RAM` (header-only `bm-ultra`) to `~12 KB Flash / ~4 KB RAM` (full hybrid-domain control).
- **Zero-heap design**: All memory is statically allocated at compile-time or during early `init()`. No `malloc` / `free`.
- **Hybrid-domain execution**: Hard real-time (HRT) control loops are isolated from soft real-time (SRT) event-driven business logic.
- **Multi-instance control**: Run multiple independent control algorithms (multi-axis servos, multi-cell BMS) with explicit resource claims and conflict detection.
- **Synchronization domains**: Phase-lock multiple PWM/ADC instances for interleaved power stages or synchronized robotic joints.
- **Cross-domain data exchange**: Lock-free triple-buffer snapshots (`bm_snapshot`) for safe HRT-to-SRT data hand-off.
- **Failure-safe startup**: `bm_ctrl_init_all()` validates all instances, resources, and bindings before execution; any failure triggers orderly rollback and safe-stop.

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
│  │  │   bm_hrt    │  │  bm_ticker  │  │      bm_ctrl_inst       │ │ │
│  │  │(scheduled  │  │  (SRT       │  │ (multi-instance control │ │ │
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

---

## Resource Tiers

| Tier | Flash | RAM | Typical MCU | Components |
|------|-------|-----|-------------|------------|
| **Ultra** | < 8 KB | < 1 KB | STM8, AVR, 8051 | `bm_ultra.h` (header-only) |
| **Nano** | 8–32 KB | 1–4 KB | CH32V003, STM32F030 | `bm_core` + optional `bm_module` |
| **Lite** | 32–128 KB | 4–16 KB | STM32F103, nRF51822 | `bm_core` + `bm_module` + `bm_channel` + `bm_shell` |
| **Control** | 32–128 KB+ | 4–16 KB+ | STM32G4, STM32F3 | All above + `bm_hrt` + `bm_ctrl_inst` + `bm_sync` |

---

## Target Domains

Zeplod Baremetal is purpose-built for **electromechanical control nodes** on resource-constrained MCUs. The following domains are ranked by native fit.

### ★★★★★ Native fit — core design targets

| Domain | Typical use case | Why it fits |
|--------|-----------------|-------------|
| **Digital power** | DC-DC, PFC, LLC, multi-phase VRM | Hardware HRT (PWM→ADC trigger) + scheduled HRT (voltage loop) + sync domains (interleaving) |
| **BMS** | Cell sampling, coulomb counting, balancing | Multi-instance model maps 1-to-1 to "pack sampler + N cells"; fault protection via comparator Break |
| **Servo / motion control** | Industrial servo, robot joints, stepper drives | Current/speed/position three-loop model matches Hardware HRT + Scheduled HRT + SRT exactly |
| **Robotics** | Manipulators, mobile robots, humanoid end-effectors | Part of the Zeplod stack; multi-axis sync + event-ID alignment with upper-layer Zeplod/Zephyr |

### ★★★★☆ Strong fit — minor gaps (protocol stacks)

| Domain | Typical use case | Gap to address |
|--------|-----------------|----------------|
| **Automotive (QM)** | BCM, thermal management, lighting, sensor nodes | Needs CAN/LIN/UDS stack integration |
| **UAV / drone** | ESC, gimbal motors, servo drives | Needs sensorless FOC and DShot/OneShot protocols |
| **Consumer / white goods** | Inverter AC, washing machine, vacuum | Needs display/touch and wireless stacks |
| **Energy / PV / ESS** | Solar inverter, PCS, EV charger module | Needs MPPT/PLL and OCPP/IEC 61850 protocols |
| **IoT / sensor nodes** | Environmental monitoring, smart metering, agriculture | Needs LoRa/BLE/Zigbee and aggressive power-management policy |

### ★★★☆☆ Moderate fit — viable with extra work

| Domain | Typical use case | Gap to address |
|--------|-----------------|----------------|
| **Industrial automation** | Remote IO, stepper drives, PID temperature | Needs Modbus/Industrial-Ethernet protocols |
| **Medical devices** | Infusion pump, ventilator motor, hospital bed | Needs IEC 62304 process and safety certification (framework itself is uncertified) |
| **Smart lighting** | LED driver, DALI/DMX decoder | Needs DALI/DMX and wireless mesh protocols |

> **Not a fit**: Commercial avionics (DO-178C), safety-critical automotive (ASIL-D), and complex GUI/HMI applications.

---

## Directory Structure

```text
zeplod-baremetal/
├── include/              # Public API headers
│   ├── bm_core.h         # Event system, mempool, critical sections, types
│   ├── bm_module.h       # Module lifecycle (optional)
│   ├── bm_channel.h      # SPSC data channel (optional)
│   ├── bm_shell.h        # Serial CLI (optional)
│   ├── bm_ultra.h        # Ultra-minimal header-only library
│   ├── bm_hrt.h          # Hard real-time dispatcher
│   ├── bm_ticker.h       # Soft real-time periodic tasks
│   ├── bm_ctrl_inst.h    # Multi-instance control abstraction
│   ├── bm_sync.h         # Synchronization domain (multi-axis phase sync)
│   ├── bm_snapshot.h     # Triple-buffer cross-domain mailbox
│   ├── bm_resource.h     # Resource claim and conflict detection
│   └── bm_hal_*.h        # HAL contracts (UART, timer, PWM, ADC, COMP, encoder, ...)
├── src/
│   ├── core/             # bm_event, bm_mempool, bm_critical, bm_wdg
│   ├── module/           # bm_module
│   ├── channel/          # bm_channel
│   ├── shell/            # bm_shell
│   ├── hrt/              # bm_hrt, bm_ticker
│   └── ctrl/             # bm_ctrl_inst, bm_resource, bm_sync
├── hal_reference/        # Reference HAL implementations
│   ├── native_sim/       # PC-native simulation (no hardware)
│   ├── qemu_cortex_m0/   # QEMU ARM Cortex-M0
│   ├── qemu_riscv32/     # QEMU RISC-V 32-bit
│   ├── stm32f0/          # STM32F0 real hardware
│   └── stm32g4/          # STM32G4 real hardware
├── examples/             # Progressive examples (see below)
├── tests/
│   ├── unit/             # Unity-based unit tests (PC native)
│   └── qemu/             # QEMU smoke tests
├── docs/
│   ├── architecture.md   # Framework architecture overview
│   ├── api/              # API reference docs
│   ├── migration/        # Migration guides (ultra→core→zephyr)
│   └── porting/          # HAL porting and IDE integration guides
├── CMakeLists.txt        # CMake build (32-bit mainstream)
└── Makefile              # Pure Makefile (8-bit toolchain friendly)
```

---

## Examples

The [`examples/`](examples/) directory contains progressive demonstrations:

| Example | Focus | Tier |
|---------|-------|------|
| [`ultra_blink`](examples/ultra_blink) | Minimal header-only event queue | Ultra |
| [`core_sensor`](examples/core_sensor) | Events, mempool, and module lifecycle | Nano |
| [`full_system`](examples/full_system) | Multi-module, event priorities, watchdog | Lite |
| [`interrupt_demo`](examples/interrupt_demo) | SysTick, peripheral IRQ, and ISR event publishing | Nano |
| [`hrt_servo_stub`](examples/hrt_servo_stub) | Hybrid-domain servo (current HRT + speed HRT + position SRT) | Control |
| [`hrt_bms_coulomb`](examples/hrt_bms_coulomb) | BMS pack sampler (ADC HRT) + cell coulomb counting (SRT) | Control |
| [`multi_axis_sync`](examples/multi_axis_sync) | Multi-instance control with synchronization domain | Control |
| [`multi_channel_bms`](examples/multi_channel_bms) | Multi-channel BMS instance model | Control |

See [`examples/README.md`](examples/README.md) for build and run instructions.

### Quick start (native simulation)

```bash
# Build and run an example on PC (no QEMU, no hardware)
cmake -B build -S examples/core_sensor -DZEPLOD_BAREMETAL_DIR=../..
cmake --build build
./build/core_sensor
```

### Quick start (QEMU Cortex-M0)

```bash
cmake -B build_qemu -S examples/interrupt_demo \
    -DZEPLOD_BAREMETAL_DIR=../.. \
    -DBOARD=qemu_cortex_m0
cmake --build build_qemu
qemu-system-arm -M microbit -kernel build_qemu/interrupt_demo.elf -nographic -serial stdio
```

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
#define BM_CONFIG_MAX_CTRL_INSTANCES         16
#define BM_CONFIG_MAX_RESOURCE_CLAIMS        64
```

Place `bm_config.h` in your application include path, or set `BM_CONFIG_FILE` in CMake.

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

See [`docs/migration/`](docs/migration/) for detailed guides.

---

## Documentation

- [`docs/architecture.md`](docs/architecture.md) — Framework architecture and dependency direction
- [`docs/api/`](docs/api/) — API reference for HRT, ticker, snapshot, control instances, sync, and resource claims
- [`docs/migration/`](docs/migration/) — Migration guides between tiers and to Zephyr
- [`docs/porting/`](docs/porting/) — HAL porting, Keil/IAR integration
- [`examples/PORTING.md`](examples/PORTING.md) — Example porting guide for custom hardware

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

[License TBD]

---

*Zeplod Baremetal is part of the Zeplod robotics stack. It is not a general-purpose RTOS replacement; it is a control-oriented framework for resource-constrained electromechanical nodes.*
