# 16 STM32 CubeMX 集成

> **本文职责**：在 STM32CubeMX / CubeIDE 工程中接入 Zeplod 的步骤要点。  
> **通用挂库模型** → [13-集成到现有工程](13-集成到现有工程.md)。

## 1. 步骤概览

| 步骤 | 操作 |
|------|------|
| ① 移植 | 复制 [`portable/template/bm_port.c`](../portable/template/bm_port.c) → `Core/Src/bm_port.c` |
| ② 配置 | 在应用 include 路径放置 `bm_config.h` |
| ③ 集成 | 添加 `Source/` 源文件或链接 `libbm_*.a`（见 [15-静态库构建](15-静态库构建.md)） |

Cube 生成的 `startup`、`HAL`、`main.c` **保持不变**；Zeplod 通过 Port 调用现有 `HAL_*`。

## 2. CMake 工程（CubeIDE / 外挂 CMake）

在厂商 `CMakeLists.txt` 末尾追加：

[`cmake/integration-snippet/CMakeLists.append.txt`](../cmake/integration-snippet/CMakeLists.append.txt)

```cmake
include(.../cmake/zeplod.cmake)
zeplod_configure(ROOT ... PROFILE event BACKEND external CONFIG Core/Inc/bm_config.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE Core/Src/bm_port.c)
zeplod_link(${CMAKE_PROJECT_NAME})
```

## 3. Keil / IAR

若使用 MDK 或 EWARM 而非 CMake，见 [18-Keil集成](18-Keil集成.md)、[19-IAR集成](19-IAR集成.md)。

## 4. 参考 Port

CMSIS 参考后端：`portable/sdk_stm32g4/`（`-DBM_STM32_CUBE_PATH=...`）。量产推荐在 `bm_port.c` 内直接调用 Cube `HAL_*`。
