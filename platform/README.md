# Platform

厂商 SDK / 寄存器实现通过 **driver API**（`include/drv/bm_drv_*.h`）对接框架契约（`include/bm/hal/bm_hal_*.h`）。

## 目录

| 路径 | 说明 |
|------|------|
| `backends/` | 各目标平台的 driver 后端 |
| `boot/qemu_cortex_m0/` | QEMU micro:bit 启动汇编、`crt0`、链接脚本 |

### 后端

| 后端 | CMake 目标 | 说明 |
|------|------------|------|
| `backends/native_sim` | `bm_backend_native_sim`（别名 `bm_hal_native`） | PC 仿真 |
| `backends/qemu_cortex_m0` | `bm_backend_qemu_cortex_m0` | QEMU + 仿真外设 |
| `backends/register_stm32g4` | `bm_backend_register_stm32g4`（别名 `bm_hal_stm32g4`） | STM32G4 寄存器参考 |
| `backends/register_esp32wroom32e` | `bm_backend_register_esp32wroom32e` | ESP32 寄存器参考 |
| `backends/register_ch32v003` | `bm_backend_register_ch32v003` | CH32V003 桩 |

## 应用链接

```cmake
set(BM_BACKEND "native_sim" CACHE STRING "")
add_subdirectory(zeplod-baremetal)
target_link_libraries(my_app PRIVATE bm_framework bm_backend_${BM_BACKEND})
```

或直接使用别名：`bm_hal_native`、`bm_hal_stm32g4` 等。

## 新增 SDK 后端

1. 在 `platform/backends/<name>/` 实现 `bm_drv_*_api` 函数表。
2. 多实例外设使用 `BM_DEVICE_DEFINE` 定义 `bm_hal_pwm_t` 等设备。
3. 单例外设导出 `const struct bm_timer_driver_api bm_drv_timer_api` 等符号。
4. `target_compile_definitions(... PUBLIC BM_DRV_HAS_BACKEND)`。
