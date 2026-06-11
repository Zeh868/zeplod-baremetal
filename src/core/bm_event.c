/**
 * @file bm_event.c
 * @brief 发布-订阅事件总线实现
 *
 * 按优先级分 FIFO 队列 + 链表订阅者；临界区保护多生产者/消费者。
 */
#include "bm_event.h"
#include "bm_critical_wrap.h"
#include "bm_log.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#if BM_CONFIG_EVENT_QUEUE_SIZE < BM_CONFIG_EVENT_PRIORITIES || \
    (BM_CONFIG_EVENT_QUEUE_SIZE % BM_CONFIG_EVENT_PRIORITIES) != 0
#error "BM_CONFIG_EVENT_QUEUE_SIZE must be divisible by BM_CONFIG_EVENT_PRIORITIES"
#endif

#define BM_EVENT_QUEUE_DEPTH_PER_PRIO \
    (BM_CONFIG_EVENT_QUEUE_SIZE / BM_CONFIG_EVENT_PRIORITIES)

#if BM_EVENT_QUEUE_DEPTH_PER_PRIO < 2 || \
    ((BM_EVENT_QUEUE_DEPTH_PER_PRIO & (BM_EVENT_QUEUE_DEPTH_PER_PRIO - 1)) != 0)
#error "BM_EVENT_QUEUE_DEPTH_PER_PRIO must be a power of two and at least 2"
#endif

/** 订阅者节点 */
typedef struct bm_subscriber {
    bm_event_callback_t           cb;
    void                         *user_data;
    bm_event_subscriber_id_t      id;
    struct bm_subscriber         *next;
} bm_subscriber_t;

/** 事件类型槽：名称 + 订阅者链表头 */
typedef struct {
    const char          *name;
    bm_subscriber_t     *head;
} bm_event_type_slot_t;

/** 队列项：事件 + 内联数据缓冲 */
typedef struct {
    bm_event_t  event;
    uint8_t     inline_data[BM_CONFIG_EVENT_INLINE_DATA_SIZE];
} bm_queue_item_t;

/** 分发阶段订阅者快照（避免回调期间链表被修改） */
typedef struct {
    bm_event_callback_t cb;
    void               *user_data;
} bm_dispatch_snap_t;

static bm_event_type_slot_t _event_types[BM_CONFIG_MAX_EVENT_TYPES];
static bm_subscriber_t      _subscribers[BM_CONFIG_MAX_EVENT_SUBSCRIBERS];
static bm_queue_item_t
    _prio_items[BM_CONFIG_EVENT_PRIORITIES][BM_EVENT_QUEUE_DEPTH_PER_PRIO];
static uint32_t             _prio_read[BM_CONFIG_EVENT_PRIORITIES];
static uint32_t             _prio_write[BM_CONFIG_EVENT_PRIORITIES];
static uint32_t             _next_subscriber_id = 1;
static uint32_t             _queue_dropped = 0;
static uint32_t             _dispatch_skipped = 0;
static bm_dispatch_snap_t   _dispatch_snap[BM_CONFIG_MAX_EVENT_SUBSCRIBERS];

static void _queue_item_copy(bm_queue_item_t *dst, const bm_queue_item_t *src) {
    bool owns_inline_data = (src->event.data == src->inline_data);

    *dst = *src;
    if (owns_inline_data) {
        dst->event.data = dst->inline_data;
    }
}

static void _prio_queues_reset(void) {
    memset(_prio_items, 0, sizeof(_prio_items));
    memset(_prio_read, 0, sizeof(_prio_read));
    memset(_prio_write, 0, sizeof(_prio_write));
}

/** 校验优先级队列读写索引在槽位掩码范围内（fail-stop） */
static int _prio_indices_valid(uint32_t read_idx, uint32_t write_idx,
                               uint32_t mask) {
    return (read_idx <= mask && write_idx <= mask) ? BM_OK : BM_ERR_INVALID;
}

void bm_event_reset(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    memset(_event_types, 0, sizeof(_event_types));
    memset(_subscribers, 0, sizeof(_subscribers));
    _prio_queues_reset();
    _next_subscriber_id = 1;
    _queue_dropped = 0;
    _dispatch_skipped = 0;
    BM_CRITICAL_EXIT(s);
    BM_LOGI("event", "event bus reset");
}

int bm_event_register_type(bm_event_type_t type, const char *name) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES || !name) {
        BM_LOGE("event", "register_type invalid type=%u", (unsigned)type);
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    if (_event_types[type].name != NULL) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("event", "type %u already registered", (unsigned)type);
        return BM_ERR_ALREADY;
    }
    _event_types[type].name = name;
    BM_CRITICAL_EXIT(s);
    BM_LOGD("event", "type %u registered as '%s'", (unsigned)type, name);
    return BM_OK;
}

int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id) {
    if (!cb || type >= BM_CONFIG_MAX_EVENT_TYPES) {
        BM_LOGE("event", "subscribe invalid args type=%u", (unsigned)type);
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    bm_subscriber_t *sub = NULL;
    int i;

    for (i = 0; i < BM_CONFIG_MAX_EVENT_SUBSCRIBERS; i++) {
        if (_subscribers[i].id == 0) {
            sub = &_subscribers[i];
            break;
        }
    }
    if (!sub) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("event", "subscribe no free slot for type=%u", (unsigned)type);
        return BM_ERR_NO_MEM;
    }
    if (_next_subscriber_id == 0u) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("event", "subscribe id exhausted");
        return BM_ERR_NO_MEM;
    }

    sub->cb = cb;
    sub->user_data = user_data;
    sub->id = _next_subscriber_id++;
    sub->next = _event_types[type].head;
    _event_types[type].head = sub;
    if (id) {
        *id = sub->id;
    }

    BM_CRITICAL_EXIT(s);
    BM_LOGD("event", "subscribed id=%u type=%u", (unsigned)sub->id, (unsigned)type);
    return BM_OK;
}

int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES || id == 0) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    bm_subscriber_t **pp = &_event_types[type].head;
    while (*pp) {
        if ((*pp)->id == id) {
            bm_subscriber_t *to_remove = *pp;

            *pp = to_remove->next;
            to_remove->id = 0;
            to_remove->cb = NULL;
            to_remove->next = NULL;
            BM_CRITICAL_EXIT(s);
            BM_LOGD("event", "unsubscribed id=%u type=%u", (unsigned)id, (unsigned)type);
            return BM_OK;
        }
        pp = &(*pp)->next;
    }

    BM_CRITICAL_EXIT(s);
    BM_LOGW("event", "unsubscribe id=%u not found", (unsigned)id);
    return BM_ERR_NOT_FOUND;
}

static int _prio_push_copy(bm_event_priority_t prio, const bm_event_t *event,
                           const void *data, size_t len) {
    uint32_t mask = BM_EVENT_QUEUE_DEPTH_PER_PRIO - 1u;
    uint32_t next;
    bm_queue_item_t *item;
    bm_irq_state_t s;

    if (prio >= BM_CONFIG_EVENT_PRIORITIES) {
        return BM_ERR_INVALID;
    }

    s = BM_CRITICAL_ENTER();
    if (_prio_indices_valid(_prio_read[prio], _prio_write[prio], mask) !=
        BM_OK) {
        BM_CRITICAL_EXIT(s);
        BM_LOGE("event", "push corrupt indices prio=%u", (unsigned)prio);
        return BM_ERR_INVALID;
    }
    next = (_prio_write[prio] + 1u) & mask;
    if (next == _prio_read[prio]) {
        _queue_dropped++;
        BM_CRITICAL_EXIT(s);
        BM_LOGW("event", "queue overflow type=%u prio=%u",
                (unsigned)event->type, (unsigned)prio);
        return BM_ERR_OVERFLOW;
    }

    item = &_prio_items[prio][_prio_write[prio] & mask];
    item->event = *event;

    if (data && len > 0u) {
        if (len <= sizeof(item->inline_data)) {
            memcpy(item->inline_data, data, len);
            item->event.data = item->inline_data;
            item->event.data_len = len;
        } else {
            BM_CRITICAL_EXIT(s);
            BM_LOGE("event", "payload too large len=%u", (unsigned)len);
            return BM_ERR_NO_MEM;
        }
    } else {
        item->event.data = NULL;
        item->event.data_len = 0;
    }

    _prio_write[prio] = next;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len) {
    bm_event_t ev;

    if (type >= BM_CONFIG_MAX_EVENT_TYPES ||
        prio >= BM_CONFIG_EVENT_PRIORITIES ||
        (len > 0u && !data)) {
        return BM_ERR_INVALID;
    }
    ev.type = type;
    ev.priority = prio;
    ev.data = NULL;
    ev.data_len = len;
    ev.source_id = 0;
    return _prio_push_copy(prio, &ev, data, len);
}

int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len) {
    /* SRT 域 ISR 专用：单核下关中断临界区可重入；禁止 HRT ISR 调用 */
    return bm_event_publish_copy(type, prio, data, len);
}

int bm_event_publish_event(const bm_event_t *event) {
    if (!event ||
        event->type >= BM_CONFIG_MAX_EVENT_TYPES ||
        event->priority >= BM_CONFIG_EVENT_PRIORITIES) {
        return BM_ERR_INVALID;
    }
    if (event->data_len > 0u && !event->data) {
        return BM_ERR_INVALID;
    }
    if (event->data_len > BM_CONFIG_EVENT_INLINE_DATA_SIZE) {
        return BM_ERR_NO_MEM;
    }
    return _prio_push_copy(event->priority, event, event->data, event->data_len);
}

int bm_event_publish_event_from_isr(const bm_event_t *event) {
    /* SRT 域 ISR 专用：单核下关中断临界区可重入；禁止 HRT ISR 调用 */
    return bm_event_publish_event(event);
}

static int _queue_pop_highest_prio(bm_queue_item_t *out) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t prio;
    uint32_t mask = BM_EVENT_QUEUE_DEPTH_PER_PRIO - 1u;

    for (prio = 0u; prio < BM_CONFIG_EVENT_PRIORITIES; ++prio) {
        if (_prio_indices_valid(_prio_read[prio], _prio_write[prio], mask) !=
            BM_OK) {
            BM_CRITICAL_EXIT(s);
            BM_LOGE("event", "pop corrupt indices prio=%u", (unsigned)prio);
            return BM_ERR_INVALID;
        }
        if (_prio_read[prio] != _prio_write[prio]) {
            uint32_t slot = _prio_read[prio] & mask;

            _queue_item_copy(out, &_prio_items[prio][slot]);
            _prio_read[prio] = (_prio_read[prio] + 1u) & mask;
            BM_CRITICAL_EXIT(s);
            return BM_OK;
        }
    }

    BM_CRITICAL_EXIT(s);
    return BM_ERR_WOULD_BLOCK;
}

uint32_t bm_event_get_dropped_count(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t dropped = _queue_dropped;
    BM_CRITICAL_EXIT(s);
    return dropped;
}

uint32_t bm_event_get_dispatch_skipped_count(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t skipped = _dispatch_skipped;
    BM_CRITICAL_EXIT(s);
    return skipped;
}

int bm_event_process(uint32_t max_events) {
    uint32_t processed = 0u;
    uint32_t i;

    for (i = 0u; i < max_events; i++) {
        bm_queue_item_t item;
        int snap_count = 0;
        int rc = _queue_pop_highest_prio(&item);
        int j;

        if (rc != BM_OK) {
            break;
        }

        if (item.event.type >= BM_CONFIG_MAX_EVENT_TYPES) {
            BM_LOGE("event", "process invalid type=%u",
                    (unsigned)item.event.type);
            bm_irq_state_t s = BM_CRITICAL_ENTER();
            _dispatch_skipped++;
            BM_CRITICAL_EXIT(s);
            processed++;
            continue;
        }

        {
            bm_irq_state_t s = BM_CRITICAL_ENTER();
            bm_subscriber_t *sub = _event_types[item.event.type].head;

            while (sub) {
                if (sub->cb) {
                    if (snap_count >= BM_CONFIG_MAX_EVENT_SUBSCRIBERS) {
                        _dispatch_skipped++;
                        BM_LOGW("event", "dispatch snap truncated type=%u",
                                (unsigned)item.event.type);
                    } else {
                        _dispatch_snap[snap_count].cb = sub->cb;
                        _dispatch_snap[snap_count].user_data = sub->user_data;
                        snap_count++;
                    }
                }
                sub = sub->next;
            }
            BM_CRITICAL_EXIT(s);
        }

        for (j = 0; j < snap_count; j++) {
            _dispatch_snap[j].cb(&item.event, _dispatch_snap[j].user_data);
        }
        processed++;
    }
    BM_LOGT("event", "processed %u events", (unsigned)processed);
    return (int)processed;
}
