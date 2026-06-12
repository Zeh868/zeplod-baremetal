/**
 * @file bm_hal_memory.c
 * @brief 内存屏障 HAL 分发层（契约 → driver API）
 */
#include "bm_drv_memory.h"
#include "bm_hal_memory.h"

#ifdef BM_DRV_HAS_BACKEND
extern const struct bm_memory_driver_api bm_drv_memory_api;
#define BM_MEMORY_DRV (&bm_drv_memory_api)
#else
static void memory_stub_release(void) {
}

static void memory_stub_full(void) {
}

static const struct bm_memory_driver_api memory_stub = {
    memory_stub_release,
    memory_stub_full,
};

#define BM_MEMORY_DRV (&memory_stub)
#endif

void bm_memory_barrier_release(void) {
    if (BM_MEMORY_DRV->barrier_release) {
        BM_MEMORY_DRV->barrier_release();
    }
}

void bm_memory_barrier_full(void) {
    if (BM_MEMORY_DRV->barrier_full) {
        BM_MEMORY_DRV->barrier_full();
    }
}
