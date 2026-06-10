# HAL 移植指南（PWM / ADC / COMP / Encoder）

## 架构

采用**方案 B：契约 + 精选参考实现**。见 [`hal-architecture.md`](hal-architecture.md)。

## 接口文件

- `include/bm_hal_pwm.h`
- `include/bm_hal_adc.h`
- `include/bm_hal_comp.h`
- `include/bm_hal_encoder.h`

## 实现清单

1. 资源句柄（`bm_hal_pwm_t` 等）描述外设实例；结构体可在 `.c` 内不透明定义
2. `bind_update` / `bind_complete` 安装 HRT 回调；`binding == NULL` 解绑
3. 解绑必须先禁用对应中断源，再清除 callback/context
4. 绑定不得隐式使能输出或启动 ADC
5. `bm_hal_pwm_request_safe_state` 进入硬件安全态（关 MOE、清零 CCR）

## 参考实现

| 平台 | 路径 | 预定义实例 |
|------|------|------------|
| 主机仿真 | `hal_reference/native_sim/bm_hal_*_sim.c` | `BM_HAL_PWM_SIM0` 等 |
| STM32G4 | `hal_reference/stm32g4/bm_hal_*_stm32g4.c` | `BM_HAL_PWM_TIM1`、`BM_HAL_ADC1`、`BM_HAL_COMP1`、`BM_HAL_ENC_TIM2` |
| QEMU 示例 | `examples/cmake/qemu_example.cmake` | 链接 native_sim 仿真源 + 平台 Timer |

STM32G4 寄存器映射见 `hal_reference/stm32g4/bm_hal_regs_stm32g4.h`；可替换为 CMSIS `stm32g4xx.h`。

## 未移植时的默认行为

`src/hal/bm_hal_*.c` 提供弱符号桩，PWM/ADC/COMP API 返回 `BM_ERR_NOT_INIT`。应用必须在链接阶段加入平台 HAL 库或自有实现。
