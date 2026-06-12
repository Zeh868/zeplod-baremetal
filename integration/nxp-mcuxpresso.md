# NXP MCUXpresso

## 顺序

1. **移植**：`source/bm_port.c` → 接 `LPUART_WriteBlocking`、`GPT_*` 等 SDK API。
2. **集成**：源码或 `integration/static-lib` 编出的 `.a`。

## 源码集成

```bash
python tools/list_sources.py --profile event --format absolute --root /path/to/zeplod
```

将库源 + `bm_port.c` 加入 MCUXpresso 的 **Zeplod** 源组；Include 用 `--list-includes`。

## CMake（SDK 风格）

```cmake
include(../zeplod-baremetal/cmake/zeplod.cmake)
zeplod_configure(ROOT ... PROFILE event BACKEND external CONFIG source/bm_config.h)
target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE source/bm_port.c)
zeplod_link(${MCUX_SDK_PROJECT_NAME})
```

## 静态库

用 `toolchain-arm-none-eabi.cmake` 或 NXP 工具链编 `integration/static-lib`，把 `libbm_*.a` 与 `include/` 拷入 SDK 工程，**Port 仍编译在应用侧**。
