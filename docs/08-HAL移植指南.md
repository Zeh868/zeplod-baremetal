# 08 HAL 移植指南

应用与 `src/` 只依赖 `include/bm_hal_*.h`；寄存器实现在 `hal_reference/` 或你的板级工程。混合域 **bind 如何接到向量 ISR** 见 [03 §3.1](03-执行域与跨域通讯.md#31-直接-hal-绑定不用-bm_ctrl_inst)。

## 三层结构

```text
include/bm_hal_*.h   契约
src/hal/             弱符号桩（未覆盖则 API 返回 BM_ERR_NOT_INIT）
hal_reference/       强符号参考实现
```

| 层级 | 职责 |
|------|------|
| 契约 | API、资源句柄类型 |
| 弱符号桩 | 链接兜底，便于 PC 编译 |
| 参考实现 | 覆盖桩，提供 ISR、寄存器操作 |

## 参考平台

| 目录 | 定位 |
|------|------|
| `native_sim/` | PC 测试与示例（含 PWM/ADC/sync 仿真） |
| `stm32g4/` | 伺服/BMS 标杆（TIM1 PWM、ADC1 注入、COMP、TIM6 HRT） |
| `ch32v003/` | Nano 级基础外设 |
| `stm32f0/` | Cortex-M0 |
| `qemu_*` | CI 冒烟 |

## CMake 链接

```cmake
target_link_libraries(my_app PRIVATE bm_hal_native bm_framework)   # PC
target_link_libraries(my_app PRIVATE bm_hal_stm32g4 bm_framework)  # G4 板
```

**务必链接平台库**。仅链 `bm_hal` 弱符号时：

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

## IDE 与示例附录

| 路径 | 内容 |
|------|------|
| [porting/keil-integration.md](porting/keil-integration.md) | Keil |
| [porting/iar-integration.md](porting/iar-integration.md) | IAR |
| [examples/PORTING.md](../examples/PORTING.md) | 示例板级替换 |

硬件验证：[reports/stm32g4-hardware-validation.md](reports/stm32g4-hardware-validation.md)（若存在）。
