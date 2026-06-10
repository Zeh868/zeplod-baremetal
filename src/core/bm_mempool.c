#include "bm_mempool.h"
#include "bm_critical_wrap.h"

void *bm_mempool_alloc(bm_mempool_t *pool) {
    if (!pool || !pool->bitmap || !pool->pool || pool->obj_size == 0) {
        return NULL;
    }

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    for (uint32_t w = 0; w < pool->bitmap_words; w++) {
        if (pool->bitmap[w] != 0xFFFFFFFFU) {
            for (int b = 0; b < 32; b++) {
                if (!(pool->bitmap[w] & (1U << b))) {
                    uint32_t idx = w * 32 + b;
                    if (idx >= pool->count) break;
                    pool->bitmap[w] |= (1U << b);
                    BM_CRITICAL_EXIT(s);
                    return (uint8_t *)pool->pool + idx * pool->obj_size;
                }
            }
        }
    }
    BM_CRITICAL_EXIT(s);
    return NULL;
}

void bm_mempool_free(bm_mempool_t *pool, void *obj) {
    if (!pool || !pool->bitmap || !pool->pool || pool->obj_size == 0 || !obj) {
        return;
    }

    uintptr_t pool_start = (uintptr_t)pool->pool;
    uintptr_t obj_address = (uintptr_t)obj;
    uintptr_t pool_end = pool_start + pool->obj_size * pool->count;
    if (obj_address < pool_start || obj_address >= pool_end) {
        return;
    }

    uintptr_t offset = obj_address - pool_start;
    if ((offset % pool->obj_size) != 0) {
        return;
    }
    uint32_t idx = (uint32_t)(offset / pool->obj_size);
    if (idx >= pool->count) return;

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    pool->bitmap[idx / 32] &= ~(1U << (idx % 32));
    BM_CRITICAL_EXIT(s);
}
