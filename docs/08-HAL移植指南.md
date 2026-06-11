# 08 HAL 移植指南

框架通过 **HAL 契约**隔离寄存器细节。应用与 `src/` 只依赖 `include/bm_hal_*.h`，具体驱动在 `hal_reference/` 或你的板级工程。

采用**方案 B：契约 + 精选参考实现**。

## 三层结构

```text
include/bm_hal_*.h     统一接口契约（所有外设）
src/hal/               默认弱符号空桩（链接时可被覆盖）
src/core/              SRT 核心与可选组件（event、mempool、module、channel、shell 等）
src/hybrid/            混合域（hrt、ticker、ctrl_inst、resource、sync）
hal_reference/         精选平台参考实现
```

| 层级 | 职责 |
|------|------|
| **契约头文件** | 声明 API、资源句柄类型；不含寄存器细节 |
| **默认桩** | `bm_hal` 静态库，弱符号实现；未移植时返回 `BM_ERR_NOT_INIT` |
| **参考实现** | 覆盖弱符号，提供可编译、可测量的真实或仿真驱动 |

其余芯片：复制最接近的参考目录，按 `include/bm_hal_*.h` 契约用厂商 LL 库实现。

## 参考平台选型

| 目录 | 定位 | 外设覆盖 |
|------|------|----------|
| `hal_reference/native_sim/` | PC 单元测试与示例 | 全量（含 PWM/ADC/COMP/Encoder） |
| `hal_reference/stm32g4/` | 伺服/BMS 标杆硬件 | TIM1 PWM、ADC1 注入、COMP1、TIM2 编码器、TIM6 HRT |
| `hal_reference/ch32v003/` | Nano 级 RISC-V | critical、timer、uart、wdg、memory |
| `hal_reference/stm32f0/` | Cortex-M0 经典 | critical、timer、uart、wdg |
| `hal_reference/qemu_*` | CI 仿真冒烟 | timer、uart、wdg、critical、memory |

## CMake 链接

```cmake
# 默认：框架自带弱符号桩
target_link_libraries(my_app PRIVATE bm_framework)

# PC / 单元测试：强符号覆盖桩
target_link_libraries(my_app PRIVATE bm_hal_native bm_framework)

# STM32G4 目标板
target_link_libraries(my_app PRIVATE bm_hal_stm32g4 bm_framework)

# CH32V003
target_link_libraries(my_app PRIVATE bm_hal_ch32v003 bm_framework)
```

链接顺序：平台 HAL 与 `bm_framework` 同链即可；GNU/LLVM 链接器优先选用强符号覆盖弱符号。

**务必链接平台库**，否则弱符号桩静默 fail-open（见 [11-安全与可靠性](11-安全与可靠性.md)）。

## 移植检查清单

1. 实现 `include/bm_hal_*.h` 中本应用用到的 API（不必实现全部外设）。
2. 定义 `bm_hal_pwm_t` 等资源句柄（可在 `.c` 内不透明定义）。
3. `bm_hal_critical_*`：与 HRT 优先级策略一致的中断屏蔽。
4. `bind_*` 解绑时先关中断再清回调；绑定不得隐式启动 ADC/PWM 输出。
5. 在 `native_sim` 上跑通单元测试，再在真机验证时序。

## PWM / ADC / COMP / Encoder（混合域外设）

控制类应用常用以下契约头文件：

- `include/bm_hal_pwm.h`
- `include/bm_hal_adc.h`
- `include/bm_hal_comp.h`
- `include/bm_hal_encoder.h`

### 实现要点

1. 资源句柄（`bm_hal_pwm_t` 等）描述外设实例；结构体可在 `.c` 内不透明定义。
2. `bind_update` / `bind_complete` 安装 HRT 回调；`binding == NULL` 表示解绑。
3. 解绑必须先禁用对应中断源，再清除 callback/context。
4. 绑定不得隐式使能 PWM 输出或启动 ADC。
5. `bm_hal_pwm_request_safe_state` 进入硬件安全态（如关 MOE、清零 CCR）。

### 参考实现与预定义实例

| 平台 | 路径 | 预定义实例 |
|------|------|------------|
| 主机仿真 | `hal_reference/native_sim/bm_hal_*_sim.c` | `BM_HAL_PWM_SIM0` 等 |
| STM32G4 | `hal_reference/stm32g4/bm_hal_*_stm32g4.c` | `BM_HAL_PWM_TIM1`、`BM_HAL_ADC1`、`BM_HAL_COMP1`、`BM_HAL_ENC_TIM2` |
| QEMU 示例 | `examples/cmake/qemu_example.cmake` | 链接 native_sim 仿真源 + 平台 Timer |

STM32G4 寄存器映射见 `hal_reference/stm32g4/bm_hal_regs_stm32g4.h`；可替换为 CMSIS `stm32g4xx.h`。

### 未移植时的默认行为

`src/hal/bm_hal_*.c` 提供弱符号桩，PWM/ADC/COMP API 返回 `BM_ERR_NOT_INIT`。

## IDE 集成（附录）

Keil / IAR 为独立工具链说明，篇幅较长，见 `porting/` 附录：

| 工具链 | 文档 |
|--------|------|
| Keil MDK-ARM | [porting/keil-integration.md](porting/keil-integration.md) |
| IAR EWARM | [porting/iar-integration.md](porting/iar-integration.md) |

## 示例级移植

分步说明：[examples/PORTING.md](../examples/PORTING.md)（时钟、链接脚本、替换 `interrupt_timer.c` 等）。

## 硬件验证报告

STM32G4 实测记录：[reports/stm32g4-hardware-validation.md](reports/stm32g4-hardware-validation.md)。
