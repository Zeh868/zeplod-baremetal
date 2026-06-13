# sdk_stm32g4 — STM32G4 CMSIS 后端

外设访问通过 **STM32Cube CMSIS 设备头**（`stm32g4xx.h`），不再维护 `bm_hal_regs_*` 手写偏移。

## CMake

```bash
cmake -B build -DBM_BACKEND=sdk_stm32g4 \
  -DBM_STM32_CUBE_PATH=/path/to/STM32CubeG4 \
  -DBM_STM32_DEVICE=STM32G474xx \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake
```

`BM_STM32_CUBE_PATH` 须指向含 `Drivers/CMSIS/Device/ST/STM32G4xx/Include` 的固件包根目录。

## 与 Cube 量产 Port 的关系

- 本后端：Zeplod 自带参考实现，直接读 CMSIS 寄存器（`TIM1->CCR1` 等）。
- 量产：仍可在 `bm_port.c` 中调用 Cube `HAL_*`，并设置 `BACKEND=external`。
