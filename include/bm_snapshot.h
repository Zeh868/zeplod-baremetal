#ifndef BM_SNAPSHOT_H
#define BM_SNAPSHOT_H

#include "bm_hal_memory.h"

#include <stdint.h>

#define BM_SNAPSHOT_NONE 0xFFu

#define BM_SNAPSHOT_DEFINE(name, type) \
    typedef struct { \
        volatile uint8_t published; \
        volatile uint8_t reading; \
        type buffer[3]; \
    } name##_snapshot_t

#define BM_SNAPSHOT_INITIALIZER \
    { .published = 0u, .reading = BM_SNAPSHOT_NONE, .buffer = { 0 } }

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

#define BM_SNAPSHOT_PUBLISH(box, value_ptr) do { \
    uint8_t _p = (box).published; \
    uint8_t _r = (box).reading; \
    uint8_t _w = bm_snapshot_choose_buffer(_p, _r); \
    (box).buffer[_w] = *(value_ptr); \
    bm_memory_barrier_release(); \
    (box).published = _w; \
} while (0)

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
