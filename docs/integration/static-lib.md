# 静态库构建

脚本：[`cmake/static-lib/CMakeLists.txt`](../../cmake/static-lib/CMakeLists.txt)

```bash
cmake -B build -S cmake/static-lib \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
    -DZEPLOD_PROFILE=event
cmake --build build
```

链接 `libbm_*.a` + 头文件；**仍须**在应用工程编译 Port（`portable/template/bm_port.c` 副本）。
