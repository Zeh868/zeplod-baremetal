/**
 * @file bm_sync_hal.h
 * @brief 同步域平台 HAL 契约
 */
#ifndef BM_SYNC_HAL_H
#define BM_SYNC_HAL_H

#include "bm_sync.h"

int bm_sync_hal_configure(const bm_sync_domain_t *domain);
int bm_sync_hal_arm(const bm_sync_domain_t *domain);
int bm_sync_hal_trigger(const bm_sync_domain_t *domain);
void bm_sync_hal_safe_stop(const bm_sync_domain_t *domain);

#endif /* BM_SYNC_HAL_H */
