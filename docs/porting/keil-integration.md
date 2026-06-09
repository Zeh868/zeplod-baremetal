# Keil MDK-ARM 集成指南

## 1. 新建工程

Project → New μVision Project → 选择目标芯片（如 STM32F030C8）。

## 2. 添加源文件

按功能层级选择：

### 仅 bm-ultra
- 无 `.c` 文件（纯头文件库）
- 添加启动文件（如 `startup_stm32f030.s`）
- 添加 `bm_hal_uart_*.c` 等 HAL 实现

### bm-core + bm-module
追加：
- `src/core/bm_critical.c`
- `src/core/bm_event.c`
- `src/core/bm_mempool.c`
- `src/module/bm_module.c`

### bm-wdg
追加：
- `src/core/bm_wdg.c`

## 3. 头文件路径

C/C++ → Include Paths：
- `include/`
- 应用目录（放置 `bm_config.h`）

## 4. 编译器选项

- C standard: C99
- 建议开启 `-O2` 或 `-Os`

## 5. 链接器设置

Keil 使用 `.sct` (Scatter) 文件，而非 GCC 的 `.ld`。

- 若使用默认分散加载：在 Target → IROM1/IRAM1 中调整地址和大小。
- 若需精确控制：将 `linker.ld` 的 `MEMORY/SECTIONS` 语义手工转为 `.sct`。

## 6. bm_config.h

复制 `bm_config.h.template` 到工程根目录，按需修改宏值。
