# HAL 架构：契约 + 精选参考实现（方案 B）

## 三层结构

```text
include/bm_hal_*.h     统一接口契约（所有外设）
src/hal/               默认弱符号空桩（链接时可被覆盖）
hal_reference/         精选平台参考实现
```

| 层级 | 职责 |
|------|------|
| **契约头文件** | 声明 API、资源句柄类型；不含寄存器细节 |
| **默认桩** | `bm_hal` 静态库，弱符号实现；未移植时返回 `BM_ERR_NOT_INIT` |
| **参考实现** | 覆盖弱符号，提供可编译、可测量的真实或仿真驱动 |

## 精选参考平台

| 目录 | 定位 | 外设覆盖 |
|------|------|----------|
| `hal_reference/native_sim/` | PC 单元测试与示例 | 全量（含 PWM/ADC/COMP/Encoder） |
| `hal_reference/stm32g4/` | 伺服/BMS 标杆硬件 | TIM1 PWM、ADC1 注入、COMP1、TIM2 编码器、TIM6 HRT |
| `hal_reference/ch32v003/` | Nano 级 RISC-V | critical、timer、uart、wdg、memory |
| `hal_reference/stm32f0/` | Cortex-M0 经典 | critical、timer、uart、wdg |
| `hal_reference/qemu_*` | CI 仿真冒烟 | timer、uart、wdg、critical、memory |

其余芯片：复制最接近的参考目录，按 `include/bm_hal_*.h` 契约用厂商 LL 库实现。

## CMake 链接方式

```cmake
# 默认：框架自带弱符号桩
target_link_libraries(my_app PRIVATE bm_framework)

# native_sim 示例/测试：强符号覆盖桩
target_link_libraries(my_app PRIVATE bm_hal_native bm_framework)

# STM32G4 目标板
target_link_libraries(my_app PRIVATE bm_hal_stm32g4 bm_framework)

# CH32V003
target_link_libraries(my_app PRIVATE bm_hal_ch32v003 bm_framework)
```

链接顺序：平台 HAL 与 `bm_framework` 同链即可；GNU/LLVM 链接器优先选用强符号覆盖弱符号。

## 移植检查清单

1. 实现 `include/bm_hal_*.h` 中本应用用到的 API
2. 定义 `bm_hal_pwm_t` 等资源句柄（可在 `.c` 内不透明定义）
3. `bind_*` 解绑时先关中断再清回调
4. 绑定不得隐式启动 ADC/PWM 输出
5. 运行 `native_sim` 单元测试对照行为

详见 [`hal-pwm-adc-comp.md`](hal-pwm-adc-comp.md)。
