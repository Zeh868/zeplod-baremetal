/* src/channel/bm_channel.c — SPSC ring buffer implementation */
#include "bm_channel.h"
#include "bm_hal_critical.h"
#include <string.h>

void bm_channel_reset(bm_channel_t *ch) {
    if (!ch) return;
    bm_irq_state_t s = bm_hal_critical_enter();
    ch->write_idx = 0;
    ch->read_idx = 0;
    bm_hal_critical_exit(s);
}

int bm_channel_send(bm_channel_t *ch, const void *data) {
    if (!ch || !data) {
        return BM_ERR_INVALID;
    }

    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t next = (ch->write_idx + 1) % ch->capacity;
    if (next == ch->read_idx) {
        bm_hal_critical_exit(s);
        return BM_ERR_OVERFLOW;
    }

    uint8_t *dst = ch->buf + ch->write_idx * ch->elem_size;
    memcpy(dst, data, ch->elem_size);
    ch->write_idx = next;
    bm_hal_critical_exit(s);
    return BM_OK;
}

int bm_channel_recv(bm_channel_t *ch, void *data) {
    if (!ch || !data) {
        return BM_ERR_INVALID;
    }

    bm_irq_state_t s = bm_hal_critical_enter();
    if (ch->read_idx == ch->write_idx) {
        bm_hal_critical_exit(s);
        return BM_ERR_WOULD_BLOCK;
    }

    const uint8_t *src = ch->buf + ch->read_idx * ch->elem_size;
    memcpy(data, src, ch->elem_size);
    ch->read_idx = (ch->read_idx + 1) % ch->capacity;
    bm_hal_critical_exit(s);
    return BM_OK;
}

uint32_t bm_channel_count(const bm_channel_t *ch) {
    if (!ch) return 0;
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t w = ch->write_idx;
    uint32_t r = ch->read_idx;
    bm_hal_critical_exit(s);
    return (w >= r) ? (w - r) : (ch->capacity - r + w);
}
