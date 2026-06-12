/**
 * @file bm_mempool.c
 * @brief 固定大小对象内存池实现
 *
 * 位图标记空闲槽，临界区保护分配/释放。
 * @author zeh (china_qzh@163.com)
 * @version 1.1
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-10       1.1            zeh            SIL-2 溢出与双释放检测
 *
 */
#include "bm_mempool.h"
#include "bm_critical_wrap.h"
#include "bm_log.h"
#include <string.h>

/**
 * @brief 校验内存池描述符基本字段
 */
static int mempool_validate_pool(const bm_mempool_t *pool) {
    uint32_t min_words;
    size_t total_bytes;

    if (!pool || !pool->bitmap || !pool->pool || pool->obj_size == 0u ||
        pool->count == 0u) {
        return BM_ERR_INVALID;
    }
    min_words = pool->count / 32u;
    if ((pool->count % 32u) != 0u) {
        min_words++;
    }
    if (pool->bitmap_words < min_words) {
        return BM_ERR_INVALID;
    }
    if (pool->obj_size > (SIZE_MAX / pool->count)) {
        return BM_ERR_INVALID;
    }
    total_bytes = pool->obj_size * pool->count;
    if (total_bytes > (size_t)UINTPTR_MAX ||
        (uintptr_t)pool->pool >
            UINTPTR_MAX - (uintptr_t)total_bytes) {
        return BM_ERR_INVALID;
    }
    return BM_OK;
}

static uint32_t mempool_required_bitmap_words(const bm_mempool_t *pool) {
    uint32_t words = pool->count / 32u;

    return words + (((pool->count % 32u) != 0u) ? 1u : 0u);
}

/**
 * @brief 计算池内存上界（溢出安全）
 */
static int mempool_pool_end(const bm_mempool_t *pool, uintptr_t *end_out) {
    size_t total_bytes;
    uintptr_t pool_start;

    if (mempool_validate_pool(pool) != BM_OK || !end_out) {
        return BM_ERR_INVALID;
    }
    total_bytes = pool->obj_size * pool->count;
    pool_start = (uintptr_t)pool->pool;
    *end_out = pool_start + total_bytes;
    return BM_OK;
}

/**
 * @brief 从内存池分配一个固定大小对象
 *
 * @param pool 内存池描述符指针
 * @return 对象指针；失败返回 NULL
 */
void *bm_mempool_alloc(bm_mempool_t *pool) {
    void *obj = NULL;
    uint32_t allocated_idx = 0u;
    uint32_t bitmap_words;

    if (mempool_validate_pool(pool) != BM_OK) {
        BM_LOGE("mempool", "alloc invalid pool");
        return NULL;
    }

    bitmap_words = mempool_required_bitmap_words(pool);
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    for (uint32_t w = 0u; w < bitmap_words; w++) {
        if (pool->bitmap[w] != 0xFFFFFFFFU) {
            for (int b = 0; b < 32; b++) {
                if (!(pool->bitmap[w] & (1U << b))) {
                    uint32_t idx = w * 32u + (uint32_t)b;
                    if (idx >= pool->count) {
                        break;
                    }

                    pool->bitmap[w] |= (1U << b);
                    obj = (uint8_t *)pool->pool +
                          (size_t)idx * pool->obj_size;
                    allocated_idx = idx;
                    BM_CRITICAL_EXIT(s);
                    memset(obj, 0, pool->obj_size);
                    BM_LOGT("mempool", "alloc slot %u",
                            (unsigned)allocated_idx);
                    return obj;
                }
            }
        }
    }
    BM_CRITICAL_EXIT(s);
    BM_LOGW("mempool", "alloc pool exhausted");
    return NULL;
}

/**
 * @brief 将对象归还内存池
 *
 * @param pool 内存池描述符指针
 * @param obj 待释放的对象指针
 */
void bm_mempool_free(bm_mempool_t *pool, void *obj) {
    uintptr_t pool_end = 0u;
    uintptr_t obj_address;
    uintptr_t offset;
    uint32_t idx;
    uint32_t word;
    uint32_t bit;

    if (mempool_validate_pool(pool) != BM_OK || !obj) {
        BM_LOGE("mempool", "free invalid args");
        return;
    }
    if (mempool_pool_end(pool, &pool_end) != BM_OK) {
        BM_LOGE("mempool", "free pool size overflow");
        return;
    }

    obj_address = (uintptr_t)obj;
    if (obj_address < (uintptr_t)pool->pool || obj_address >= pool_end) {
        BM_LOGE("mempool", "free out of range ptr=%p", obj);
        return;
    }

    offset = obj_address - (uintptr_t)pool->pool;
    if ((offset % pool->obj_size) != 0u) {
        BM_LOGE("mempool", "free misaligned ptr=%p", obj);
        return;
    }
    idx = (uint32_t)(offset / pool->obj_size);
    if (idx >= pool->count) {
        BM_LOGE("mempool", "free idx out of range ptr=%p", obj);
        return;
    }

    word = idx / 32u;
    bit = idx % 32u;
    if (word >= pool->bitmap_words) {
        BM_LOGE("mempool", "free bitmap word overflow idx=%u", (unsigned)idx);
        return;
    }

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    if (!(pool->bitmap[word] & (1U << bit))) {
        BM_CRITICAL_EXIT(s);
        BM_LOGE("mempool", "free double-free slot %u", (unsigned)idx);
        return;
    }
    pool->bitmap[word] &= ~(1U << bit);
    BM_CRITICAL_EXIT(s);
    BM_LOGT("mempool", "free slot %u", (unsigned)idx);
}

void bm_mempool_reset(bm_mempool_t *pool) {
    uint32_t bitmap_words;

    if (mempool_validate_pool(pool) != BM_OK) {
        BM_LOGE("mempool", "reset invalid pool");
        return;
    }

    bitmap_words = mempool_required_bitmap_words(pool);
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    memset(pool->bitmap, 0, (size_t)bitmap_words * sizeof(uint32_t));
    BM_CRITICAL_EXIT(s);
    BM_LOGT("mempool", "reset all slots free");
}
