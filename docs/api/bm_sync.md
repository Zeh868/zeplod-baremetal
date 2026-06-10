# bm_sync — 多实例硬件同步域

头文件：`include/bm_sync.h`  
实现：`src/ctrl/bm_sync.c` + 平台 `bm_sync_hal_*.c`

## 概述

通过主定时器与 per-member 相位偏移，协调多个控制实例的 PWM 更新与采样时序。框架 API 校验参数并委托 HAL 完成 ITR/触发路由。

## 类型

### `bm_sync_domain_t`

| 字段 | 说明 |
|------|------|
| `name` | 同步域名称 |
| `master_timer` | 主 `bm_hal_timer_t` 指针 |
| `members` | `bm_ctrl_inst_t` 指针数组 |
| `phase_ticks` | 各成员相对主定时器的相位（tick） |
| `member_count` | 成员数量 |

## API

### `bm_sync_configure(domain)`

配置主从关系与相位表，委托 HAL 设置硬件触发链。  
返回：`BM_OK`；`BM_ERR_INVALID`；HAL 错误码。

### `bm_sync_arm(domain)`

武装同步域，准备一次性或周期性触发。

### `bm_sync_trigger(domain)`

触发同步域，按 `phase_ticks` 依次启动各成员 slot。  
QEMU 参考 HAL 在 `trigger` 时同步执行成员 `step`（仿真用）。

### `bm_sync_safe_stop(domain)`

安全停止并复位 HAL 状态。

## 生命周期

```
configure → arm → trigger (可重复) → safe_stop
```

须在 `bm_ctrl_start_all` 之后、`trigger` 之前完成 `configure` 与 `arm`。

## 平台实现

| 平台 | 文件 |
|------|------|
| native_sim | `hal_reference/native_sim/bm_sync_hal_native.c` |
| QEMU M0 | `hal_reference/qemu_cortex_m0/bm_sync_hal_qemu.c` |
| STM32G4 | `hal_reference/stm32g4/bm_sync_hal_stm32g4.c` |

真实 MCU 上 `trigger` 通过定时器比较/ITR 硬件路由；仿真环境可退化为软件逐步调用。
