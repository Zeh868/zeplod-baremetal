# STM32 CubeMX

1. **移植**：复制 [`portable/template/bm_port.c`](../../portable/template/bm_port.c) → `Core/Src/bm_port.c`
2. **集成**：见 [README.md](README.md)

Cube 的 `startup` / `HAL` / `main.c` 保持不变。

CMake 片段：[`cmake/integration-snippet/CMakeLists.append.txt`](../../cmake/integration-snippet/CMakeLists.append.txt)
