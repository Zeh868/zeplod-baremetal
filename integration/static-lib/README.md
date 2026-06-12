# 静态库构建

产出 **库本体**（`libbm_core.a`、`libbm_hal.a` …），**不含** Port。

## 构建

```bash
cmake -B build -S integration/static-lib \
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake \
    -DZEPLOD_PROFILE=event \
    -DZEPLOD_CONFIG=../../integration/cmake-snippet/bm_config.h
cmake --build build
```

库位于 `build/_zeplod_lib_build/**/`。

## 在厂商工程中使用

1. 拷贝 `include/` 下 `bm/`、`drv/` 到工程或加 -I 指向 Zeplod 根。
2. 链接上一步生成的 `libbm_*.a`（PROFILE 须与配置一致）。
3. 在应用工程编译 [`../port/bm_port.c`](../port/bm_port.c)（移植层）。
4. 提供应用侧 `bm_config.h`。

库以 `BM_BACKEND=external` 编译，链接期由 Port 提供 `bm_drv_*_api`。
