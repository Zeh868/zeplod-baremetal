#ifndef BM_MEMPOOL_H
#define BM_MEMPOOL_H

#include "bm_types.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t *bitmap;
    void     *pool;
    size_t    obj_size;
    uint32_t  count;
    uint32_t  bitmap_words;
} bm_mempool_t;

#define BM_MEMPOOL_DEFINE(name, type, cnt) \
    static uint32_t _bm_pool_bitmap_##name[((cnt) + 31U) / 32U] = {0}; \
    static type _bm_pool_storage_##name[(cnt)]; \
    static bm_mempool_t name = { \
        .bitmap = _bm_pool_bitmap_##name, \
        .pool = _bm_pool_storage_##name, \
        .obj_size = sizeof(type), \
        .count = (cnt), \
        .bitmap_words = ((cnt) + 31U) / 32U \
    }

void *bm_mempool_alloc(bm_mempool_t *pool);
void bm_mempool_free(bm_mempool_t *pool, void *obj);

#endif /* BM_MEMPOOL_H */
