# HAL 移植指南（PWM / ADC / COMP）

## 接口文件

- `include/bm_hal_pwm.h`
- `include/bm_hal_adc.h`
- `include/bm_hal_comp.h`
- `include/bm_hal_encoder.h`

## 实现清单

1. 资源句柄（`bm_hal_pwm_t` 等）描述外设实例
2. `bind_update` / `bind_complete` 安装 HRT 回调；`binding == NULL` 解绑
3. 绑定不得隐式使能输出或启动 ADC
4. `bm_hal_pwm_request_safe_state` 进入硬件安全态

## 参考

- 主机仿真：`hal_reference/native_sim/bm_hal_*_sim.c`
- QEMU：`examples/cmake/qemu_example.cmake` 链接 sim 源 + 平台 Timer
- STM32G4：待补充完整 TIM/ADC/COMP 寄存器实现
