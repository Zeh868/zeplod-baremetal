#ifndef BM_CRITICAL_WRAP_H
#define BM_CRITICAL_WRAP_H

#include "bm_hal_critical.h"

#ifndef BM_CONFIG_ENABLE_PRIORITY_MASK
#define BM_CONFIG_ENABLE_PRIORITY_MASK 0
#endif

#ifndef BM_HAL_HAS_PRIORITY_MASK
#define BM_HAL_HAS_PRIORITY_MASK 0
#endif

#if BM_CONFIG_ENABLE_PRIORITY_MASK
#if !BM_HAL_HAS_PRIORITY_MASK
#error "BM_CONFIG_ENABLE_PRIORITY_MASK requires BM_HAL_HAS_PRIORITY_MASK"
#endif
#ifndef BM_CONFIG_HRT_PRIORITY_THRESHOLD
#define BM_CONFIG_HRT_PRIORITY_THRESHOLD 4
#endif
#define BM_CRITICAL_ENTER() \
    bm_hal_critical_enter_below((uint8_t)BM_CONFIG_HRT_PRIORITY_THRESHOLD)
#define BM_CRITICAL_EXIT(state) bm_hal_critical_exit_below(state)
#else
#define BM_CRITICAL_ENTER() bm_hal_critical_enter()
#define BM_CRITICAL_EXIT(state) bm_hal_critical_exit(state)
#endif

#endif /* BM_CRITICAL_WRAP_H */
