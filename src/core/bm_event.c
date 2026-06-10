#include "bm_event.h"
#include "bm_critical_wrap.h"
#include <stdbool.h>
#include <string.h>

#ifndef BM_CONFIG_MAX_EVENT_TYPES
#define BM_CONFIG_MAX_EVENT_TYPES       16
#endif

#ifndef BM_CONFIG_MAX_EVENT_SUBSCRIBERS
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS 32
#endif

#ifndef BM_CONFIG_EVENT_QUEUE_SIZE
#define BM_CONFIG_EVENT_QUEUE_SIZE      16
#endif

#ifndef BM_CONFIG_EVENT_PRIORITIES
#define BM_CONFIG_EVENT_PRIORITIES      4
#endif

#if BM_CONFIG_EVENT_QUEUE_SIZE < 2 || \
    (BM_CONFIG_EVENT_QUEUE_SIZE & (BM_CONFIG_EVENT_QUEUE_SIZE - 1)) != 0
#error "BM_CONFIG_EVENT_QUEUE_SIZE must be a power of two and at least 2"
#endif

#ifndef BM_CONFIG_EVENT_INLINE_DATA_SIZE
#define BM_CONFIG_EVENT_INLINE_DATA_SIZE 8
#endif

typedef struct bm_subscriber {
    bm_event_callback_t           cb;
    void                         *user_data;
    bm_event_subscriber_id_t      id;
    struct bm_subscriber         *next;
} bm_subscriber_t;

typedef struct {
    const char          *name;
    bm_subscriber_t     *head;
} bm_event_type_slot_t;

typedef struct {
    bm_event_t  event;
    uint8_t     inline_data[BM_CONFIG_EVENT_INLINE_DATA_SIZE];
} bm_queue_item_t;

static bm_event_type_slot_t _event_types[BM_CONFIG_MAX_EVENT_TYPES];
static bm_subscriber_t      _subscribers[BM_CONFIG_MAX_EVENT_SUBSCRIBERS];
static bm_queue_item_t      _event_queue[BM_CONFIG_EVENT_QUEUE_SIZE];
static uint32_t             _queue_write = 0;
static uint32_t             _queue_read = 0;
static uint32_t             _next_subscriber_id = 1;

static void _queue_item_copy(bm_queue_item_t *dst, const bm_queue_item_t *src) {
    bool owns_inline_data = (src->event.data == src->inline_data);
    *dst = *src;
    if (owns_inline_data) {
        dst->event.data = dst->inline_data;
    }
}

void bm_event_reset(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    memset(_event_types, 0, sizeof(_event_types));
    memset(_subscribers, 0, sizeof(_subscribers));
    memset(_event_queue, 0, sizeof(_event_queue));
    _queue_write = 0;
    _queue_read = 0;
    _next_subscriber_id = 1;
    BM_CRITICAL_EXIT(s);
}

int bm_event_register_type(bm_event_type_t type, const char *name) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES || !name) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    _event_types[type].name = name;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id) {
    if (!cb || type >= BM_CONFIG_MAX_EVENT_TYPES || !id) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    bm_subscriber_t *sub = NULL;
    for (int i = 0; i < BM_CONFIG_MAX_EVENT_SUBSCRIBERS; i++) {
        if (_subscribers[i].id == 0) {
            sub = &_subscribers[i];
            break;
        }
    }
    if (!sub) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_NO_MEM;
    }

    sub->cb = cb;
    sub->user_data = user_data;
    sub->id = _next_subscriber_id++;
    sub->next = _event_types[type].head;
    _event_types[type].head = sub;
    *id = sub->id;

    BM_CRITICAL_EXIT(s);
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
            return BM_OK;
        }
        pp = &(*pp)->next;
    }

    BM_CRITICAL_EXIT(s);
    return BM_ERR_NOT_FOUND;
}

static int _queue_push_copy(const bm_event_t *event, const void *data, size_t len) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t next = (_queue_write + 1) & mask;
    if (next == _queue_read) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_OVERFLOW;
    }

    bm_queue_item_t *item = &_event_queue[_queue_write & mask];
    item->event = *event;

    if (data && len > 0) {
        if (len <= sizeof(item->inline_data)) {
            memcpy(item->inline_data, data, len);
            item->event.data = item->inline_data;
        } else {
            BM_CRITICAL_EXIT(s);
            return BM_ERR_NO_MEM;
        }
    } else {
        item->event.data = NULL;
        item->event.data_len = 0;
    }

    _queue_write = next;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES ||
        prio >= BM_CONFIG_EVENT_PRIORITIES ||
        (len > 0 && !data)) {
        return BM_ERR_INVALID;
    }
    bm_event_t ev = {
        .type = type,
        .priority = prio,
        .data = NULL,
        .data_len = len,
        .source_id = 0
    };
    return _queue_push_copy(&ev, data, len);
}

int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len) {
    return bm_event_publish_copy(type, prio, data, len);
}

int bm_event_publish_event(const bm_event_t *event) {
    if (!event ||
        event->type >= BM_CONFIG_MAX_EVENT_TYPES ||
        event->priority >= BM_CONFIG_EVENT_PRIORITIES ||
        (event->data_len > 0 && !event->data)) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t next = (_queue_write + 1) & mask;
    if (next == _queue_read) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_OVERFLOW;
    }
    _event_queue[_queue_write & mask].event = *event;
    _queue_write = next;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

int bm_event_publish_event_from_isr(const bm_event_t *event) {
    /* The platform critical section serializes ISR and main-loop producers. */
    return bm_event_publish_event(event);
}

static int _queue_pop_highest_prio(bm_queue_item_t *out) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    if (_queue_read == _queue_write) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_WOULD_BLOCK;
    }

    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t best_idx = 0;
    uint8_t best_prio = 0xFF;
    bool found = false;

    for (uint32_t i = _queue_read; i != _queue_write; i = (i + 1) & mask) {
        bm_event_t *ev = &_event_queue[i & mask].event;
        if (ev->priority < best_prio) {
            best_prio = ev->priority;
            best_idx = i;
            found = true;
        }
    }

    if (!found) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_WOULD_BLOCK;
    }

    uint32_t read_slot = _queue_read & mask;
    uint32_t best_slot = best_idx & mask;
    _queue_item_copy(out, &_event_queue[best_slot]);
    if (best_slot != read_slot) {
        _queue_item_copy(&_event_queue[best_slot], &_event_queue[read_slot]);
    }
    _queue_read = (_queue_read + 1) & mask;

    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

int bm_event_process(uint32_t max_events) {
    uint32_t processed = 0;
    for (uint32_t i = 0; i < max_events; i++) {
        bm_queue_item_t item;
        int rc = _queue_pop_highest_prio(&item);
        if (rc != BM_OK) break;

        bm_subscriber_t *sub = _event_types[item.event.type].head;
        while (sub) {
            bm_subscriber_t *next = sub->next;
            if (sub->cb) {
                sub->cb(&item.event, sub->user_data);
            }
            sub = next;
        }
        processed++;
    }
    return (int)processed;
}
