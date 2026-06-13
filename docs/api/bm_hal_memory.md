# bm_hal_memory — 内存屏障 HAL

头文件：`include/bm_hal_memory.h`

## 概述

为 `bm_snapshot` 等无锁结构提供 release / full 内存屏障，保证写操作对读者的可见性与读写有序性。须在链接阶段提供平台实现，不可使用未定义的弱符号默认值做生产快照。

## API

### `bm_memory_barrier_release()`

写屏障：确保调用点之前的写操作在后续读操作之前对其他核/ISR 可见。用于 `BM_SNAPSHOT_PUBLISH` 更新 `published` 之前。

### `bm_memory_barrier_full()`

全屏障：读写均有序。用于 `BM_SNAPSHOT_READ` 标记 `reading` 之后、拷贝之前。

## 参考实现

| 平台 | 文件 | 实现 |
|------|------|------|
| native_sim | `platform/backends/native_sim/bm_drv_singleton_native.c` | 编译器屏障 |
| QEMU Cortex-M0 | `platform/backends/qemu_cortex_m0/bm_drv_singleton_qemu.c` | 编译器屏障 |
| STM32G4 | `portable/sdk_stm32g4/bm_drv_singleton_stm32g4.c` | `dmb` / `dsb` |
| CH32V003 | `platform/backends/register_ch32v003/bm_drv_singleton_ch32v003.c` | RISC-V `fence` |

## 移植要点

1. ARM Cortex-M：在 `bm_drv_singleton_*.c` 中实现 `bm_drv_memory_api`。
2. 单核无缓存 MCU：release 可用编译器 `memory` clobber；多核或 DMA 一致性场景须用硬件屏障。
3. RISC-V：参考 `register_ch32v003` 后端中的 `fence` 实现。

## 与快照的配合

```
写者: buffer[w] = data → barrier_release() → published = w
读者: reading = p → barrier_full() → 校验 published → copy → reading = NONE
```

缺少正确屏障可能导致读者读到撕裂数据或陈旧缓存。
