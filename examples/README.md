# Zeplod Baremetal Examples

三个渐进式示例，演示从 `bm-ultra` 到 `bm-core` 全栈的用法。

## 示例关系

```
ultra_blink  →  core_sensor  →  full_system
(bm-ultra)      (bm-core+module)  (core+module+wdg)
```

## 前置要求

- `arm-none-eabi-gcc`
- `qemu-system-arm`
- CMake 3.20+

## 构建与运行

### ultra_blink（bm-ultra 最简）

```bash
cd ultra_blink
cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/ultra_blink.elf --semihosting -nographic -serial stdio
```

### core_sensor（事件 + 内存池 + 模块）

```bash
cd core_sensor
cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/core_sensor.elf --semihosting -nographic -serial stdio
```

### full_system（全栈）

```bash
cd full_system
cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/full_system.elf --semihosting -nographic -serial stdio
```

## 拷贝到自己的项目

每个示例目录是自包含的。复制后修改 `CMakeLists.txt` / `Makefile` 中的 `ZEPLOD_ROOT` 路径即可。

## 验证输出

每个示例最终输出 `EXAMPLE_XXX: PASS`。
