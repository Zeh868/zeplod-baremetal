# NXP MCUXpresso

1. **移植**：`source/bm_port.c`（自 [`portable/template/bm_port.c`](../../portable/template/bm_port.c)）
2. **集成**：源码或 [static-lib.md](static-lib.md)

```cmake
zeplod_configure(ROOT ... PROFILE event BACKEND external CONFIG source/bm_config.h)
target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE source/bm_port.c)
zeplod_link(${MCUX_SDK_PROJECT_NAME})
```
