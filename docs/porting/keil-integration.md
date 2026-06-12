# Keil MDK-ARM Integration

Zeplod integrates like FreeRTOS: **port** in the app, then **library** as source or `.lib`.

See [docs/integration/README.md](../integration/README.md).

1. **Port** — copy `portable/template/bm_port.c`, wire to vendor HAL.
2. **Library** — add `Source/` files (source mode) or link `libbm_*.a`.

## Include Paths

1. Application directory with `bm_config.h` (first)
2. `include/bm/{common,core,hybrid,hal,ultra}` and `include/drv` for Port
3. Vendor CMSIS / device headers

## Library Sources

```bash
python tools/list_sources.py --profile event --format keil --root-macro ZEPLOD_ROOT
python tools/list_sources.py --profile event --list-includes --format keil --root-macro ZEPLOD_ROOT
```

Also add your `bm_port.c`. Static libs: build `cmake/static-lib/`, link `libbm_*.a`, still compile Port in app.

## HAL

Implement `bm_drv_*_api` in `bm_port.c`. Reference: `portable/register_stm32g4/`.
