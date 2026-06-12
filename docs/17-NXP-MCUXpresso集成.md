# 17 NXP MCUXpresso 集成

> **本文职责**：在 MCUXpresso SDK 工程中接入 Zeplod。  
> **通用挂库模型** → [13-集成到现有工程](13-集成到现有工程.md)。

## 1. 步骤概览

| 步骤 | 操作 |
|------|------|
| ① 移植 | `source/bm_port.c`（自 [`portable/template/bm_port.c`](../portable/template/bm_port.c) 复制并改 HAL） |
| ② 配置 | `source/bm_config.h` |
| ③ 集成 | 源码列表或 [15-静态库构建](15-静态库构建.md) |

## 2. CMake 片段

```cmake
include(.../cmake/zeplod.cmake)
zeplod_configure(ROOT ... PROFILE event BACKEND external CONFIG source/bm_config.h)
target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE source/bm_port.c)
zeplod_link(${MCUX_SDK_PROJECT_NAME})
```

`BACKEND external` 表示库不携带平台代码，符号由应用侧 Port 提供。

## 3. 其他工具链

Keil / IAR 工程见 [18-Keil集成](18-Keil集成.md)、[19-IAR集成](19-IAR集成.md)。
