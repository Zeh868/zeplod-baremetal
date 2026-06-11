/**
 * @file bm_ultra.c
 * @brief Ultra 剖面事件队列单例实现
 */
#include "bm_ultra.h"
#include "bm_critical_wrap.h"

#include <string.h>

#if BM_CONFIG_ULTRA_QUEUE_DEPTH < 2 || \
    ((BM_CONFIG_ULTRA_QUEUE_DEPTH & (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1)) != 0)
#error "BM_CONFIG_ULTRA_QUEUE_DEPTH must be a power of two and at least 2"
#endif

static bm_ultra_queue_t _bm_ultra_q;
static uint32_t         _bm_ultra_dropped;

static int ultra_indices_valid(uint8_t read_idx, uint8_t write_idx) {
    uint8_t mask = (uint8_t)(BM_CONFIG_ULTRA_QUEUE_DEPTH - 1u);

    return (read_idx <= mask && write_idx <= mask) ? BM_OK : BM_ERR_INVALID;
}

int bm_ultra_queue_push(const bm_ultra_queue_item_t *item) {
    uint8_t next;
    bm_irq_state_t s;

    if (!item) {
        return BM_ERR_INVALID;
    }

    s = BM_CRITICAL_ENTER();
    if (ultra_indices_valid(_bm_ultra_q.read_idx, _bm_ultra_q.write_idx) !=
        BM_OK) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_INVALID;
    }
    next = (uint8_t)((_bm_ultra_q.write_idx + 1u) &
                     (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1u));
    if (next == _bm_ultra_q.read_idx) {
        _bm_ultra_dropped++;
        BM_CRITICAL_EXIT(s);
        return BM_ERR_OVERFLOW;
    }
    _bm_ultra_q.items[_bm_ultra_q.write_idx] = *item;
    _bm_ultra_q.write_idx = next;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

int bm_ultra_queue_pop(bm_ultra_queue_item_t *item) {
    bm_irq_state_t s;

    if (!item) {
        return BM_ERR_INVALID;
    }

    s = BM_CRITICAL_ENTER();
    if (ultra_indices_valid(_bm_ultra_q.read_idx, _bm_ultra_q.write_idx) !=
        BM_OK) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_INVALID;
    }
    if (_bm_ultra_q.read_idx == _bm_ultra_q.write_idx) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_WOULD_BLOCK;
    }
    *item = _bm_ultra_q.items[_bm_ultra_q.read_idx];
    _bm_ultra_q.read_idx = (uint8_t)((_bm_ultra_q.read_idx + 1u) &
                                       (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1u));
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

void bm_ultra_queue_reset(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    memset(&_bm_ultra_q, 0, sizeof(_bm_ultra_q));
    _bm_ultra_dropped = 0u;
    BM_CRITICAL_EXIT(s);
}

uint32_t bm_ultra_get_dropped_count(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t dropped = _bm_ultra_dropped;
    BM_CRITICAL_EXIT(s);
    return dropped;
}

uint8_t bm_ultra_queue_count(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint8_t count;

    if (ultra_indices_valid(_bm_ultra_q.read_idx, _bm_ultra_q.write_idx) !=
        BM_OK) {
        BM_CRITICAL_EXIT(s);
        return 0u;
    }
    count = (uint8_t)((_bm_ultra_q.write_idx - _bm_ultra_q.read_idx) &
                      (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1u));
    BM_CRITICAL_EXIT(s);
    return count;
}

const bm_ultra_queue_t *bm_ultra_queue_state(void) {
    return &_bm_ultra_q;
}

uint8_t bm_ultra_process(void) {
    bm_ultra_queue_item_t item;
    bm_ultra_callback_t cb;

    if (bm_ultra_queue_pop(&item) != BM_OK) {
        return 0u;
    }
    if (item.event_type >= BM_CONFIG_ULTRA_MAX_EVENT_TYPES) {
        return 1u;
    }
    cb = _bm_ultra_callbacks[item.event_type];
    if (cb != NULL) {
        cb(item.data, item.data_len);
    }
    return 1u;
}
