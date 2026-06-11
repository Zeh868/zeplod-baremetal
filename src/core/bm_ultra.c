/**
 * @file bm_ultra.c
 * @brief Ultra 剖面事件队列单例实现
 */
#include "bm_ultra.h"

#include <string.h>

static bm_ultra_queue_t _bm_ultra_q;

int bm_ultra_queue_push(const bm_ultra_queue_item_t *item) {
    uint8_t next;

    if (!item) {
        return -1;
    }
    next = (uint8_t)((_bm_ultra_q.write_idx + 1u) &
                     (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1u));
    if (next == _bm_ultra_q.read_idx) {
        return -1;
    }
    _bm_ultra_q.items[_bm_ultra_q.write_idx] = *item;
    _bm_ultra_q.write_idx = next;
    return 0;
}

int bm_ultra_queue_pop(bm_ultra_queue_item_t *item) {
    if (!item) {
        return -1;
    }
    if (_bm_ultra_q.read_idx == _bm_ultra_q.write_idx) {
        return -1;
    }
    *item = _bm_ultra_q.items[_bm_ultra_q.read_idx];
    _bm_ultra_q.read_idx = (uint8_t)((_bm_ultra_q.read_idx + 1u) &
                                     (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1u));
    return 0;
}

void bm_ultra_queue_reset(void) {
    memset(&_bm_ultra_q, 0, sizeof(_bm_ultra_q));
}

const bm_ultra_queue_t *bm_ultra_queue_state(void) {
    return &_bm_ultra_q;
}
