# IAR EWARM 集成指南

## 1. 新建工程

File → New → Workspace → 创建工程并选择目标芯片。

## 2. 添加源文件

源文件选择与 Keil 相同（见 `keil-integration.md` §2）。

## 3. 头文件路径

Project → Options → C/C++ Compiler → Preprocessor → Additional include directories：
- `include/`
- 应用目录（放置 `bm_config.h`）

## 4. 编译器选项

- C standard: C99 (`--c99` 或 `-e`，取决于 IAR 版本)
- 建议优化等级：Balanced (`-Ohs`) 或 Size (`-Ohz`)

## 5. 链接器设置

IAR 使用 `.icf` 文件，语法与 GCC `.ld` 不同。

- 建议先使用 IAR 默认 linker configuration（`lnnn.icf`）。
- 如需调整：在 Project → Options → Linker → Config 中编辑 `.icf`，修改 `define region` 的 RAM/FLASH 范围。

## 6. bm_config.h

复制 `bm_config.h.template` 到工程根目录，按需修改宏值。
