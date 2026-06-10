/**
 * @file bm_mempool.c
 * @brief 固定大小对象内存池实现
 *
 * 位图标记空闲槽，临界区保护分配/释放。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#include "bm_mempool.h"
#include "bm_critical_wrap.h"
#include "bm_log.h"

/**
 * @brief 从内存池分配一个固定大小对象
 *
 * @param pool 内存池描述符指针
 * @return 对象指针；失败返回 NULL
 */
void *bm_mempool_alloc(bm_mempool_t *pool) {
    if (!pool || !pool->bitmap || !pool->pool || pool->obj_size == 0) {
        BM_LOGE("mempool", "alloc invalid pool");
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
                    BM_LOGT("mempool", "alloc slot %u", (unsigned)idx);
                    return (uint8_t *)pool->pool + idx * pool->obj_size;
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
    if (!pool || !pool->bitmap || !pool->pool || pool->obj_size == 0 || !obj) {
        BM_LOGE("mempool", "free invalid args");
        return;
    }

    uintptr_t pool_start = (uintptr_t)pool->pool;
    uintptr_t obj_address = (uintptr_t)obj;
    uintptr_t pool_end = pool_start + pool->obj_size * pool->count;
    if (obj_address < pool_start || obj_address >= pool_end) {
        BM_LOGE("mempool", "free out of range ptr=%p", obj);
        return;
    }

    uintptr_t offset = obj_address - pool_start;
    if ((offset % pool->obj_size) != 0) {
        BM_LOGE("mempool", "free misaligned ptr=%p", obj);
        return;
    }
    uint32_t idx = (uint32_t)(offset / pool->obj_size);
    if (idx >= pool->count) return;

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    pool->bitmap[idx / 32] &= ~(1U << (idx % 32));
    BM_CRITICAL_EXIT(s);
    BM_LOGT("mempool", "free slot %u", (unsigned)idx);
}
