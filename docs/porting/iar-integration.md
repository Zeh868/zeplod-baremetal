# IAR EWARM Integration

Zeplod Baremetal can be added to an EWARM project as C99 source code. A modern
EWARM release with C99 support is recommended.

## 1. Add Include Paths

In `Project -> Options -> C/C++ Compiler -> Preprocessor`, add:

1. The application directory containing `bm_config.h`
2. `<zeplod-baremetal>/include`
3. The MCU vendor CMSIS and device include directories

Keep the application directory first. The framework ships
`include/bm_config.h` with defaults; the application copy must override it.
Start from `bm_config.h.template`.

## 2. Add Framework Sources

Mandatory core:

```text
src/core/bm_critical.c
src/core/bm_event.c
src/core/bm_mempool.c
```

Optional sources:

```text
src/core/bm_module.c
src/core/bm_channel.c
src/core/bm_shell.c
src/core/bm_wdg.c
```

Generate the list with:

```bash
python tools/list_sources.py --format iar \
  --enable-module ON --enable-channel OFF \
  --enable-shell OFF --enable-wdg ON
```

`bm_ultra.h` is header-only.

## 3. Provide Platform HAL

The core requires a target implementation of:

```c
bm_irq_state_t bm_hal_critical_enter(void);
void bm_hal_critical_exit(bm_irq_state_t state);
```

For STM32F0 projects, the reference implementation uses CMSIS intrinsics and
is compatible with vendor device headers:

```text
hal_reference/stm32f0/bm_hal_critical_stm32f0.c
```

The shell additionally needs UART HAL functions. The watchdog component needs
timer and watchdog HAL functions. Hardware projects must not include native or
QEMU HAL implementations.

## 4. Compiler and Linker

- Select the C99 language mode supported by the installed EWARM version.
- Use the vendor startup file and CMSIS system initialization.
- Use the device `.icf` linker configuration. The QEMU `linker.ld` is not
  applicable.
- Size optimization is recommended.

The framework does not require compiler-specific weak symbols, custom linker
sections, heap allocation, or constructors.

## 5. Application Contracts

When the module component is enabled, define exactly one module table:

```c
const bm_module_t _bm_module_table[] = {
    /* application modules */
};
const uint32_t _bm_module_count =
    sizeof(_bm_module_table) / sizeof(_bm_module_table[0]);
```

When using `bm_ultra.h`, define exactly one callback table with
`BM_ULTRA_CALLBACK_TABLE_DEFINE`.

## 6. Integration Checklist

- Application `bm_config.h` precedes the framework include directory.
- All source files compile with the same configuration.
- Required HAL symbols have exactly one implementation.
- Event queue depth is a power of two.
- Copied event payloads fit `BM_CONFIG_EVENT_INLINE_DATA_SIZE`.
