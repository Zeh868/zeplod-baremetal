#ifndef BM_SYNC_H
#define BM_SYNC_H

#include "bm_ctrl_inst.h"
#include "bm_types.h"

typedef struct bm_hal_timer bm_hal_timer_t;

typedef struct {
    const char *name;
    const bm_hal_timer_t *master_timer;
    const bm_ctrl_inst_t *const *members;
    const uint32_t *phase_ticks;
    uint32_t member_count;
} bm_sync_domain_t;

int bm_sync_configure(const bm_sync_domain_t *domain);
int bm_sync_arm(const bm_sync_domain_t *domain);
int bm_sync_trigger(const bm_sync_domain_t *domain);
void bm_sync_safe_stop(const bm_sync_domain_t *domain);

#endif /* BM_SYNC_H */
