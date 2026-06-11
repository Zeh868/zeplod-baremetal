# Architecture

The framework is organized around one mandatory core and opt-in components.

## Dependency Direction

```text
application
  |
  +-- bm_module / bm_channel / bm_shell / bm_wdg
  |        |
  |        +-- bm_core
  |
  +-- bm_core
           |
           +-- HAL contracts
```

Public contracts are split by responsibility:

- `bm_types.h`: shared result codes and low-level scalar types.
- `bm_event.h`: event publish/subscribe API.
- `bm_mempool.h`: fixed-size memory pool API.
- `bm_atomic.h`: critical-section-backed atomic helpers.
- `bm_core.h`: compatibility umbrella for applications that want all core APIs.

Component headers depend on `bm_types.h`, not on the complete core API. This
keeps compile-time dependencies explicit and prevents HAL headers from
depending back on the framework umbrella.

## Source Layout

```text
src/core/    SRT framework and optional components (module, channel, shell, wdg)
src/hybrid/  HRT scheduler, ticker, multi-instance control, sync domain
src/hal/     weak HAL stubs overridden by platform ports
```

## CMake Targets

- `bm_core`: mandatory event, memory pool, and critical-section code.
- `bm_module`, `bm_channel`, `bm_shell`, `bm_wdg`: optional component targets.
- `bm_framework`: aggregate interface target containing all enabled targets.
- `bm_config`: compile settings shared by every framework target.

Applications should link only the component targets they use. Link
`bm_framework` when a product intentionally enables the complete configured
feature set.

Set `BM_CONFIG_FILE` to an application configuration header. If a
`bm_config.h` exists at the repository root, CMake selects it automatically.
The header is force-included while compiling framework and consumer targets,
so configuration macros are applied consistently.

## State Ownership

Core event state is currently a single static instance to preserve the small
RAM footprint and existing API. Call `bm_event_reset()` during application
startup and test setup to establish a deterministic initial state.
