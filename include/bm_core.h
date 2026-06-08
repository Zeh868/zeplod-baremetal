#ifndef BM_CORE_H
#define BM_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BM_OK                0
#define BM_ERR_NO_MEM       -1
#define BM_ERR_NOT_FOUND    -2
#define BM_ERR_WOULD_BLOCK  -3
#define BM_ERR_INVALID      -4
#define BM_ERR_BUSY         -5
#define BM_ERR_OVERFLOW     -6
#define BM_ERR_NOT_INIT     -7
#define BM_ERR_ALREADY      -8

typedef uint8_t bm_event_type_t;
typedef uint8_t bm_event_priority_t;
typedef uint32_t bm_event_subscriber_id_t;
typedef uint32_t bm_irq_state_t;
typedef volatile uint32_t bm_atomic_t;

typedef struct {
    bm_event_type_t      type;
    bm_event_priority_t  priority;
    const void          *data;
    size_t               data_len;
    uint8_t              source_id;
} bm_event_t;

typedef void (*bm_event_callback_t)(const bm_event_t *event, void *user_data);

typedef struct {
    uint32_t *bitmap;
    void     *pool;
    size_t    obj_size;
    uint32_t  count;
    uint32_t  bitmap_words;
} bm_mempool_t;

#define BM_MEMPOOL_DEFINE(name, type, cnt) \
    static uint32_t _bm_pool_bitmap_##name[(cnt + 31) / 32] = {0}; \
    static type _bm_pool_storage_##name[cnt]; \
    static bm_mempool_t name = { \
        .bitmap = _bm_pool_bitmap_##name, \
        .pool = _bm_pool_storage_##name, \
        .obj_size = sizeof(type), \
        .count = cnt, \
        .bitmap_words = (cnt + 31) / 32 \
    }

void *bm_mempool_alloc(bm_mempool_t *pool);
void bm_mempool_free(bm_mempool_t *pool, void *obj);

/* Event system API */
int bm_event_register_type(bm_event_type_t type, const char *name);
int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id);
int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id);
int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len);
int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len);
int bm_event_publish_event(const bm_event_t *event);
int bm_event_publish_event_from_isr(const bm_event_t *event);
int bm_event_process(uint32_t max_events);

#endif /* BM_CORE_H */
