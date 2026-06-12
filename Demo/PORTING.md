# Example Porting Guide

The example build files target QEMU Cortex-M0. For a hardware project, keep
the application files and replace the QEMU startup, linker script, and HAL
implementations with files for the target MCU.

## Shared Application Files

- `common/example_support.c`: small UART output and delay helpers.
- `<example>/main.c`: framework usage and application flow.
- `<example>/bm_config.h`: resource limits for that example.

`interrupt_demo` also uses `interrupt_timer.c`, which is intentionally
platform-specific and should be replaced during a port.

## Framework Components

| Example | Framework sources |
|---|---|
| `ultra_blink` | Header-only `bm_ultra.h` |
| `core_sensor` | Core event/mempool/critical code and `bm_module.c` |
| `full_system` | Core, module lifecycle, and watchdog |
| `interrupt_demo` | Core event/mempool/critical code |

The authoritative source selection is in each example's short
`CMakeLists.txt` or `Makefile`. Shared QEMU build policy lives in
`cmake/qemu_example.cmake` and `qemu_example.mk`.

For Keil and IAR integration details, see `docs/porting/`.
