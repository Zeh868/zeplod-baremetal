#ifndef BM_ATOMIC_H
#define BM_ATOMIC_H

#include "bm_types.h"

uint32_t bm_atomic_load(bm_atomic_t *value);
void bm_atomic_store(bm_atomic_t *value, uint32_t new_value);
uint32_t bm_atomic_inc(bm_atomic_t *value);

#endif /* BM_ATOMIC_H */
