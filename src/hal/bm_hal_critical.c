/**
 * @file bm_hal_critical.c
 * @brief 临界区 HAL 分发层（契约 → driver API）
 */
#include "bm_drv_critical.h"
#include "bm_hal_critical.h"
#include "bm_types.h"

#ifdef BM_DRV_HAS_BACKEND
extern const struct bm_critical_driver_api bm_drv_critical_api;
#define BM_CRITICAL_DRV (&bm_drv_critical_api)
#else
static bm_irq_state_t critical_stub_enter(void) {
    return 0;
}

static void critical_stub_exit(bm_irq_state_t state) {
    (void)state;
}

#if BM_HAL_HAS_PRIORITY_MASK
static bm_irq_state_t critical_stub_enter_below(uint8_t threshold) {
    (void)threshold;
    return 0;
}

static void critical_stub_exit_below(bm_irq_state_t previous_state) {
    (void)previous_state;
}
#endif

static const struct bm_critical_driver_api critical_stub = {
    critical_stub_enter,
    critical_stub_exit,
#if BM_HAL_HAS_PRIORITY_MASK
    critical_stub_enter_below,
    critical_stub_exit_below,
#endif
};

#define BM_CRITICAL_DRV (&critical_stub)
#endif

bm_irq_state_t bm_hal_critical_enter(void) {
    if (!BM_CRITICAL_DRV->enter) {
        return 0;
    }
    return BM_CRITICAL_DRV->enter();
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    if (BM_CRITICAL_DRV->exit) {
        BM_CRITICAL_DRV->exit(state);
    }
}

#if BM_HAL_HAS_PRIORITY_MASK
bm_irq_state_t bm_hal_critical_enter_below(uint8_t threshold) {
    if (!BM_CRITICAL_DRV->enter_below) {
        return 0;
    }
    return BM_CRITICAL_DRV->enter_below(threshold);
}

void bm_hal_critical_exit_below(bm_irq_state_t previous_state) {
    if (BM_CRITICAL_DRV->exit_below) {
        BM_CRITICAL_DRV->exit_below(previous_state);
    }
}
#endif
