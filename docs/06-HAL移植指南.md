# 06 HAL 移植指南

框架通过 **HAL 契约**隔离寄存器细节。应用与 `src/` 只依赖 `include/bm_hal_*.h`，具体驱动在 `hal_reference/` 或你的板级工程。

## 三层结构

```text
include/bm_hal_*.h     接口契约
src/hal/               弱符号空桩（未移植时 BM_ERR_NOT_INIT）
hal_reference/         官方参考实现（可覆盖弱符号）
```

| 层级 | 职责 |
|------|------|
| 契约头文件 | API、资源句柄类型；无寄存器 |
| 默认桩 | 保证链接通过；提醒未实现 |
| 参考实现 | PC 仿真、STM32G4 伺服、CH32V003 Nano 等 |

详细架构：[porting/hal-architecture.md](porting/hal-architecture.md)。

## 参考平台选型

| 目录 | 用途 |
|------|------|
| `native_sim` | PC 单元测试、示例默认 |
| `stm32g4` | 伺服/BMS：PWM、ADC 注入、COMP、编码器、HRT 定时器 |
| `stm32f0` | Cortex-M0 基础：critical、timer、uart、wdg |
| `ch32v003` | RISC-V Nano 级 |
| `qemu_cortex_m0` / `qemu_riscv32` | CI 冒烟 |

新芯片：复制最接近的参考目录，按契约用厂商 LL/HAL 实现。

## CMake 链接

```cmake
# PC / 单元测试
target_link_libraries(my_app PRIVATE bm_hal_native bm_framework)

# STM32G4 目标板
target_link_libraries(my_app PRIVATE bm_hal_stm32g4 bm_framework)
```

**强符号覆盖弱符号**；务必链接平台库，否则静默 fail-open（见 [09-安全与可靠性](09-安全与可靠性.md)）。

## 移植检查清单

1. 实现应用用到的 `bm_hal_*.h` API（不必实现全部外设）。
2. 资源句柄（如 `bm_hal_pwm_t`）可在 `.c` 内不透明定义。
3. `bm_hal_critical_*`：与 HRT 优先级策略一致的中断屏蔽。
4. **HRT 绑定**：`bind_*` 解绑先关中断再清回调；绑定不隐式启动 PWM/ADC。
5. PWM/ADC/COMP 细节：[porting/hal-pwm-adc-comp.md](porting/hal-pwm-adc-comp.md)。
6. 在 `native_sim` 上跑通单元测试，再在真机验证时序。

## IDE 集成

| 工具链 | 文档 |
|--------|------|
| Keil MDK | [porting/keil-integration.md](porting/keil-integration.md) |
| IAR EWARM | [porting/iar-integration.md](porting/iar-integration.md) |

## 示例级移植

分步说明：[examples/PORTING.md](../examples/PORTING.md)（时钟、链接脚本、替换 `interrupt_timer.c` 等）。

## 硬件验证报告

STM32G4 实测记录：[reports/stm32g4-hardware-validation.md](reports/stm32g4-hardware-validation.md)。
