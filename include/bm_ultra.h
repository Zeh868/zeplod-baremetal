#ifndef BM_ULTRA_H
#define BM_ULTRA_H

#include <bm_config.h>

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef BM_CONFIG_ULTRA_MAX_EVENT_TYPES
#define BM_CONFIG_ULTRA_MAX_EVENT_TYPES     8
#endif

#ifndef BM_CONFIG_ULTRA_QUEUE_DEPTH
#define BM_CONFIG_ULTRA_QUEUE_DEPTH         8
#endif

#ifndef BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE 8
#endif

typedef uint8_t bm_event_type_t;

typedef struct {
    bm_event_type_t event_type;
    uint8_t         data[BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE];
    uint8_t         data_len;
} bm_ultra_queue_item_t;

typedef void (*bm_ultra_callback_t)(const void *data, uint8_t len);

/* Compile-time callback table.
 * User defines this in ONE .c file:
 *   static const bm_ultra_callback_t _bm_ultra_callbacks[BM_CONFIG_ULTRA_MAX_EVENT_TYPES] = {
 *       [EVENT_TEMP] = on_temp,
 *   };
 */
#define BM_ULTRA_CALLBACK_TABLE_DEFINE(...) \
    const bm_ultra_callback_t _bm_ultra_callbacks[BM_CONFIG_ULTRA_MAX_EVENT_TYPES] = { __VA_ARGS__ }

#define BM_ULTRA_CB(event_type, callback) \
    [event_type] = callback

typedef struct {
    bm_ultra_queue_item_t items[BM_CONFIG_ULTRA_QUEUE_DEPTH];
    uint8_t write_idx;
    uint8_t read_idx;
} bm_ultra_queue_t;

static bm_ultra_queue_t _bm_ultra_q;

static inline int _bm_ultra_queue_push(const bm_ultra_queue_item_t *item) {
    uint8_t next = (_bm_ultra_q.write_idx + 1) & (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1);
    if (next == _bm_ultra_q.read_idx) {
        return -1; /* full */
    }
    _bm_ultra_q.items[_bm_ultra_q.write_idx] = *item;
    _bm_ultra_q.write_idx = next;
    return 0;
}

static inline int _bm_ultra_queue_pop(bm_ultra_queue_item_t *item) {
    if (_bm_ultra_q.read_idx == _bm_ultra_q.write_idx) {
        return -1; /* empty */
    }
    *item = _bm_ultra_q.items[_bm_ultra_q.read_idx];
    _bm_ultra_q.read_idx = (_bm_ultra_q.read_idx + 1) & (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1);
    return 0;
}

static inline void bm_ultra_init(void) {
    memset(&_bm_ultra_q, 0, sizeof(_bm_ultra_q));
}

static inline int bm_ultra_publish(bm_event_type_t type,
                                    const void *data, uint8_t len) {
    if (len > BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE) {
        return -1;
    }
    bm_ultra_queue_item_t item;
    item.event_type = type;
    item.data_len = len;
    if (len > 0 && data != NULL) {
        memcpy(item.data, data, len);
    }
    return _bm_ultra_queue_push(&item);
}

static inline int bm_ultra_publish_from_isr(bm_event_type_t type,
                                             const void *data, uint8_t len) {
    return bm_ultra_publish(type, data, len);
}

static inline uint8_t bm_ultra_event_count(void) {
    return (uint8_t)((_bm_ultra_q.write_idx - _bm_ultra_q.read_idx)
                     & (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1));
}

extern const bm_ultra_callback_t _bm_ultra_callbacks[BM_CONFIG_ULTRA_MAX_EVENT_TYPES];

static inline uint8_t bm_ultra_process(void) {
    bm_ultra_queue_item_t item;
    if (_bm_ultra_queue_pop(&item) != 0) {
        return 0;
    }
    bm_ultra_callback_t cb = _bm_ultra_callbacks[item.event_type];
    if (cb != NULL) {
        cb(item.data, item.data_len);
    }
    return 1;
}

#endif /* BM_ULTRA_H */
