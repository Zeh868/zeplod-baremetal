/**
 * @file bm_mempool.h
 * @brief 固定大小对象内存池
 *
 * 基于位图追踪空闲槽位，支持 O(n) 分配与释放。
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
#ifndef BM_MEMPOOL_H
#define BM_MEMPOOL_H

#include "bm_types.h"

#include <stddef.h>
#include <stdint.h>

/** 内存池控制块 */
typedef struct {
    uint32_t *bitmap;
    void     *pool;
    size_t    obj_size;
    uint32_t  count;
    uint32_t  bitmap_words;
} bm_mempool_t;

/** 静态定义内存池实例
 *  示例：BM_MEMPOOL_DEFINE(my_pool, my_type_t, 16); */
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

/**
 * @brief 从内存池分配一个对象
 *
 * @param pool 内存池控制块指针
 * @return 对象指针（已清零）；池满时返回 NULL
 */
void *bm_mempool_alloc(bm_mempool_t *pool);

/**
 * @brief 将对象归还内存池
 *
 * @param pool 内存池控制块指针
 * @param obj 待释放的对象指针
 */
void bm_mempool_free(bm_mempool_t *pool, void *obj);

#endif /* BM_MEMPOOL_H */
