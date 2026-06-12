# 14 Port 移植层

> **本文职责**：说明量产 Port 文件要实现的驱动 API 符号与参考位置。  
> **不负责**：如何把库链进 Keil/IAR → [13-集成到现有工程](13-集成到现有工程.md)；HAL 契约语义 → [08-HAL移植指南](08-HAL移植指南.md)。

## 1. 定位

类比 FreeRTOS `portable/<compiler>/<mcu>/port.c`：Port 是**应用工程侧**的平台胶水，不在预编译静态库内。驱动 API 头文件为 `include/bm_drv_*.h`（与 `bm_hal_*.h` 同在 `include/` 根目录）。

**模板：** [`portable/template/bm_port.c`](../portable/template/bm_port.c)

**参考实现：** `portable/sdk_<mcu>/`（PC 开发可用 `BM_BACKEND=native_sim`）

## 2. 必须实现

| 符号 | 用途 |
|------|------|
| `bm_drv_critical_api` | 临界区进入/退出 |
| `bm_drv_memory_api` | 内存屏障 |

## 3. 按功能选用

| 符号 | 组件 |
|------|------|
| `bm_drv_timer_api` | 看门狗、HRT |
| `bm_drv_uart_api` | 日志、Shell |
| `bm_drv_pwm_api` 等 | 混合域外设 |

实现时对接厂商 SDK（Cube `HAL_*`、ESP-IDF `driver/*`）；可参考 `portable/sdk_stm32g4/`、`portable/sdk_esp32_idf/`。

## 4. 集成检查清单

1. 从 `portable/template/bm_port.c` 复制到应用 `Core/Src/` 或 `source/`。
2. 按 PROFILE 实现本应用用到的 `bm_drv_*_api`。
3. 在 `native_sim` 或 QEMU 上跑通单元测试（若适用）。
4. 真机验证 HRT 时序与 ISR 上下文（混合域见 [03 §3.1](03-执行域与跨域通讯.md#31-直接-hal-绑定不用-bm_ctrl_inst)）。

## 5. 相关文档

| 主题 | 文档 |
|------|------|
| `portable/` 目录说明 | [08-HAL移植指南](08-HAL移植指南.md) |
| Cube / SDK 挂库步骤 | [13](13-集成到现有工程.md)、[16](16-STM32-CubeMX集成.md)、[17](17-NXP-MCUXpresso集成.md) |
