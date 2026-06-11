#ifndef BM_SYNC_HAL_NATIVE_H
#define BM_SYNC_HAL_NATIVE_H

#include "bm_sync.h"

void bm_sync_hal_native_reset(void);
void bm_sync_hal_native_fail_configure(int enabled);
void bm_sync_hal_native_fail_arm(int enabled);
void bm_sync_hal_native_fail_trigger(int enabled);
int bm_sync_hal_native_triggered(void);
int bm_sync_hal_native_safe_stop_count(void);
bm_sync_state_t bm_sync_hal_native_configure_observed_state(void);
bm_sync_state_t bm_sync_hal_native_arm_observed_state(void);
bm_sync_state_t bm_sync_hal_native_trigger_observed_state(void);
bm_sync_state_t bm_sync_hal_native_safe_stop_observed_state(void);

#endif /* BM_SYNC_HAL_NATIVE_H */
