/* include/bm_channel.h — lightweight SPSC data channel */
#ifndef BM_CHANNEL_H
#define BM_CHANNEL_H

#include "bm_core.h"

#ifndef BM_CONFIG_CHANNEL_MAX_CHANNELS
#define BM_CONFIG_CHANNEL_MAX_CHANNELS 4
#endif

typedef struct {
    uint8_t *buf;
    uint32_t elem_size;
    uint32_t capacity;
    volatile uint32_t write_idx;
    volatile uint32_t read_idx;
} bm_channel_t;

/* Static channel definition macro.
 * Example:
 *   typedef struct { uint16_t adc; uint32_t timestamp; } sensor_t;
 *   BM_CHANNEL_DEFINE(my_chan, sensor_t, 8);
 */
#define BM_CHANNEL_DEFINE(name, type, depth) \
    static uint8_t _bm_channel_buf_##name[(depth) * sizeof(type)] = {0}; \
    static bm_channel_t name = { \
        .buf = _bm_channel_buf_##name, \
        .elem_size = sizeof(type), \
        .capacity = (depth), \
        .write_idx = 0, \
        .read_idx = 0 \
    }

/* Reset channel to empty state */
void bm_channel_reset(bm_channel_t *ch);

/* Non-blocking send (ISR-safe). Returns BM_OK or BM_ERR_OVERFLOW. */
int bm_channel_send(bm_channel_t *ch, const void *data);

/* Non-blocking receive (ISR-safe). Returns BM_OK or BM_ERR_WOULD_BLOCK. */
int bm_channel_recv(bm_channel_t *ch, void *data);

/* Number of elements currently in channel */
uint32_t bm_channel_count(const bm_channel_t *ch);

/* Total capacity */
static inline uint32_t bm_channel_capacity(const bm_channel_t *ch) {
    return ch ? ch->capacity : 0;
}

/* Query helpers */
static inline bool bm_channel_is_empty(const bm_channel_t *ch) {
    return ch ? (ch->write_idx == ch->read_idx) : true;
}

static inline bool bm_channel_is_full(const bm_channel_t *ch) {
    if (!ch) return true;
    uint32_t next = (ch->write_idx + 1) % ch->capacity;
    return next == ch->read_idx;
}

#endif /* BM_CHANNEL_H */
