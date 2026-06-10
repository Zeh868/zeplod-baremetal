#ifndef BM_RESOURCE_H
#define BM_RESOURCE_H

#include "bm_types.h"

#include <stdint.h>

typedef enum {
    BM_RESOURCE_PWM,
    BM_RESOURCE_ADC_GROUP,
    BM_RESOURCE_TIMER,
    BM_RESOURCE_DMA_CHANNEL,
    BM_RESOURCE_GPIO,
    BM_RESOURCE_COMP,
    BM_RESOURCE_IRQ
} bm_resource_kind_t;

typedef enum {
    BM_RESOURCE_EXCLUSIVE,
    BM_RESOURCE_OWNER,
    BM_RESOURCE_SHARED_READ,
    BM_RESOURCE_SHARED_COORDINATED
} bm_resource_access_t;

typedef struct {
    bm_resource_kind_t kind;
    uintptr_t key;
    bm_resource_access_t access;
    uintptr_t share_group;
    const char *name;
} bm_resource_claim_t;

int bm_resource_check_conflicts(const bm_resource_claim_t *const *claims,
                                const uint32_t *claim_counts,
                                uint32_t instance_count);

#endif /* BM_RESOURCE_H */
