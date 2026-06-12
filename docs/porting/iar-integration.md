# IAR EWARM Integration

Same model as FreeRTOS: Port in application, library as source or `.a`.

See [docs/integration/README.md](../integration/README.md).

```bash
python tools/list_sources.py --profile event --format iar --root-macro ZEPLOD_ROOT
python tools/list_sources.py --profile event --list-includes --format iar --root-macro ZEPLOD_ROOT
```

Add `portable/template/bm_port.c` (copied and adapted) separately.

Reference Port: `portable/register_stm32g4/bm_drv_singleton_stm32g4.c`.
