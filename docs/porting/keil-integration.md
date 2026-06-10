# Keil MDK-ARM Integration

Zeplod Baremetal is distributed as C99 source code. Add the required source
files directly to an existing MDK project; no prebuilt library is required.

Arm Compiler 6 is recommended. Arm Compiler 5 has only partial C99 support and
is not part of the verified toolchain set.

## 1. Add Include Paths

In `Options for Target -> C/C++ (AC6) -> Include Paths`, add paths in this
order:

1. The application configuration directory containing `bm_config.h`
2. `<zeplod-baremetal>/include`
3. The MCU vendor CMSIS and device include directories

The application path must come first so its `bm_config.h` overrides the
framework defaults in `include/bm_config.h`.

Copy `bm_config.h.template` into the application directory and tune the
resource limits. Every translation unit must see the same configuration.

## 2. Add Framework Sources

The mandatory core is:

```text
src/core/bm_critical.c
src/core/bm_event.c
src/core/bm_mempool.c
```

Add optional components only when used:

```text
src/module/bm_module.c
src/channel/bm_channel.c
src/shell/bm_shell.c
src/core/bm_wdg.c
```

Generate a source list with:

```bash
python tools/list_sources.py --format keil \
  --enable-module ON --enable-channel OFF \
  --enable-shell OFF --enable-wdg ON
```

`bm_ultra.h` is header-only and does not require framework `.c` files.

## 3. Provide Platform HAL

The core requires:

```c
bm_irq_state_t bm_hal_critical_enter(void);
void bm_hal_critical_exit(bm_irq_state_t state);
```

Use `hal_reference/stm32f0/bm_hal_critical_stm32f0.c` for an STM32F0 CMSIS
project, or provide an equivalent implementation for the target MCU. Do not
add native or QEMU HAL files to a hardware project.

Optional component dependencies:

| Component | Required HAL |
|---|---|
| Core, module, channel | Critical section |
| Shell | Critical section and UART |
| Watchdog | Critical section, timer, and watchdog |

The UART, timer, and watchdog STM32F0 files are interface stubs and must be
completed for the selected device and board.

## 4. Compiler and Linker

- Select a C99-compatible language mode.
- Size optimization is recommended.
- Use the device pack startup file and CMSIS system file.
- Use the normal Keil scatter file or target memory configuration. The QEMU
  `linker.ld` is not used by Keil.

The framework uses no heap, constructors, custom linker sections, or runtime
registration. Module registration is an application-defined
`_bm_module_table`.

## 5. Minimal Application

```c
#include "bm_event.h"

static void on_event(const bm_event_t *event, void *user_data)
{
    (void)event;
    (void)user_data;
}

int main(void)
{
    bm_event_subscriber_id_t subscriber_id;

    bm_event_reset();
    bm_event_register_type(1, "APP_EVENT");
    bm_event_subscribe(1, on_event, 0, &subscriber_id);

    for (;;) {
        bm_event_process(4);
    }
}
```

## 6. Integration Checklist

- Application `bm_config.h` is found before the framework default.
- Exactly one target implementation of each required HAL symbol is linked.
- `BM_CONFIG_EVENT_QUEUE_SIZE` is a power of two and at least 2.
- `BM_CONFIG_EVENT_INLINE_DATA_SIZE` fits every `bm_event_publish_copy`
  payload.
- All framework and application files use the same configuration macros.
