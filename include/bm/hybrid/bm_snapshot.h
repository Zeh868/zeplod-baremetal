/**
 * @file bm_snapshot.h
 * @brief 三缓冲无锁快照（header-only）
 *
 * 写者发布、读者拷贝，通过 published/reading 索引避免读写竞争。
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
#ifndef BM_SNAPSHOT_H
#define BM_SNAPSHOT_H

#include "hal/bm_hal_memory.h"

#include <stdint.h>

/** reading 槽位空闲标记 */
#define BM_SNAPSHOT_NONE 0xFFu

/** 定义快照盒类型（含 3 份 buffer）
 *  示例：BM_SNAPSHOT_DEFINE(my_snap, sensor_data_t); */
#define BM_SNAPSHOT_DEFINE(name, type) \
    typedef struct { \
        volatile uint8_t published; \
        volatile uint8_t reading; \
        type buffer[3]; \
    } name##_snapshot_t

#define BM_SNAPSHOT_INITIALIZER \
    { .published = 0u, .reading = BM_SNAPSHOT_NONE, .buffer = { 0 } }

/**
 * @brief 选取可写入的 buffer 索引（避开 published 与 reading）
 *
 * @param published 当前已发布 buffer 索引
 * @param reading 当前读者正在读取的 buffer 索引
 * @return 可用于写入的 buffer 索引
 */
static inline uint8_t bm_snapshot_choose_buffer(uint8_t published,
                                                uint8_t reading) {
    uint8_t i;

    for (i = 0u; i < 3u; ++i) {
        if (i != published && i != reading) {
            return i;
        }
    }
    return 0u;
}

/** 发布快照（写者侧） */
#define BM_SNAPSHOT_PUBLISH(box, value_ptr) do { \
    uint8_t _p = (box).published; \
    uint8_t _r = (box).reading; \
    uint8_t _w = bm_snapshot_choose_buffer(_p, _r); \
    (box).buffer[_w] = *(value_ptr); \
    bm_memory_barrier_release(); \
    (box).published = _w; \
} while (0)

/** 读取快照（读者侧，一致性拷贝） */
#define BM_SNAPSHOT_READ(box, out_ptr) do { \
    uint8_t _p; \
    do { \
        _p = (box).published; \
        (box).reading = _p; \
        bm_memory_barrier_full(); \
    } while ((box).published != _p); \
    *(out_ptr) = (box).buffer[_p]; \
    bm_memory_barrier_release(); \
    (box).reading = BM_SNAPSHOT_NONE; \
} while (0)

#endif /* BM_SNAPSHOT_H */
