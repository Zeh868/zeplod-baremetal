/**
 * @file bm_ultra.h
 * @brief 超轻量事件队列（单 TU 队列 + 编译期回调表）
 *
 * 队列状态在 bm_ultra.c 中单例实现；回调表须在单个 .c 中实例化。
 */
#ifndef BM_ULTRA_H
#define BM_ULTRA_H

#include <bm_config.h>
#include <bm_types.h>

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef uint8_t bm_event_type_t;

/** 队列元素（内联固定长度数据） */
typedef struct {
    bm_event_type_t event_type;
    uint8_t         data[BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE];
    uint8_t         data_len;
} bm_ultra_queue_item_t;

/** 事件分发回调 */
typedef void (*bm_ultra_callback_t)(const void *data, uint8_t len);

/** 编译期定义回调表（须在单个 .c 文件中实例化） */
#define BM_ULTRA_CALLBACK_TABLE_DEFINE(...) \
    const bm_ultra_callback_t _bm_ultra_callbacks[BM_CONFIG_ULTRA_MAX_EVENT_TYPES] = { __VA_ARGS__ }

#define BM_ULTRA_CB(event_type, callback) \
    [event_type] = callback

/** 环形队列控制块 */
typedef struct {
    bm_ultra_queue_item_t items[BM_CONFIG_ULTRA_QUEUE_DEPTH];
    uint8_t write_idx;
    uint8_t read_idx;
} bm_ultra_queue_t;

int      bm_ultra_queue_push(const bm_ultra_queue_item_t *item);
int      bm_ultra_queue_pop(bm_ultra_queue_item_t *item);
void     bm_ultra_queue_reset(void);
uint32_t bm_ultra_get_dropped_count(void);
uint8_t  bm_ultra_queue_count(void);
const bm_ultra_queue_t *bm_ultra_queue_state(void);

static inline void bm_ultra_init(void) {
    bm_ultra_queue_reset();
}

static inline int bm_ultra_publish(bm_event_type_t type,
                                    const void *data, uint8_t len) {
    bm_ultra_queue_item_t item;

    if (len > BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE) {
        return BM_ERR_NO_MEM;
    }
    item.event_type = type;
    item.data_len = len;
    if (len > 0u && data != NULL) {
        memcpy(item.data, data, len);
    }
    return bm_ultra_queue_push(&item);
}

/** SRT 域 ISR 专用：单核下关中断临界区可重入；禁止 HRT ISR 调用 */
static inline int bm_ultra_publish_from_isr(bm_event_type_t type,
                                             const void *data, uint8_t len) {
    return bm_ultra_publish(type, data, len);
}

static inline uint8_t bm_ultra_event_count(void) {
    return bm_ultra_queue_count();
}

extern const bm_ultra_callback_t _bm_ultra_callbacks[BM_CONFIG_ULTRA_MAX_EVENT_TYPES];

static inline uint8_t bm_ultra_process(void) {
    bm_ultra_queue_item_t item;
    bm_ultra_callback_t cb;

    if (bm_ultra_queue_pop(&item) != BM_OK) {
        return 0u;
    }
    cb = _bm_ultra_callbacks[item.event_type];
    if (cb != NULL) {
        cb(item.data, item.data_len);
    }
    return 1u;
}

#endif /* BM_ULTRA_H */
