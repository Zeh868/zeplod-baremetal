# STM32 CubeMX

## 顺序

1. **移植**：`Core/Src/bm_port.c`（从 [`port/bm_port.c`](port/bm_port.c) 复制）→ 接 `HAL_TIM_*`、`HAL_UART_*`。
2. **集成**：源码或静态库二选一（见 [README.md](README.md)）。

Cube 生成的 `startup`、`HAL`、`main.c` **不要动**。

## 源码集成（Keil / IAR / Cube CMake）

```text
MyProject/
├── Core/Src/bm_port.c          ← 移植
├── Core/Inc/bm_config.h
├── ThirdParty/zeplod-baremetal/
└── Drivers/ ...                  ← Cube 生成
```

- Include：`bm_config.h` 目录 + `zeplod-baremetal/include/bm/{common,core,hybrid,hal,ultra}` + `include/drv`
- 源文件：`list_sources.py --profile event` 输出 + `bm_port.c` + Cube 源

## CMake（Cube 已导出）

```cmake
include(ThirdParty/zeplod-baremetal/cmake/zeplod.cmake)
zeplod_configure(ROOT .../zeplod-baremetal PROFILE event BACKEND external
                 CONFIG Core/Inc/bm_config.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE Core/Src/bm_port.c)
zeplod_link(${CMAKE_PROJECT_NAME})
```

## 定时器 ISR

在 `HAL_TIM_PeriodElapsedCallback` 或 `TIMx_IRQHandler` 中：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        bm_port_timer_isr();
    }
}
```
