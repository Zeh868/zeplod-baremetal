#include "bm_core.h"
#include "bm_hal_critical.h"
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
    uint8_t     inline_data[8]; /* publish_copy ≤8B inline */
} bm_queue_item_t;

static bm_event_type_slot_t _event_types[BM_CONFIG_MAX_EVENT_TYPES];
static bm_subscriber_t      _subscribers[BM_CONFIG_MAX_EVENT_SUBSCRIBERS];
static bm_queue_item_t      _event_queue[BM_CONFIG_EVENT_QUEUE_SIZE];
static uint32_t             _queue_write = 0;
static uint32_t             _queue_read = 0;
static uint32_t             _next_subscriber_id = 1;

int bm_event_register_type(bm_event_type_t type, const char *name) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = bm_hal_critical_enter();
    _event_types[type].name = name;
    bm_hal_critical_exit(s);
    return BM_OK;
}

int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id) {
    if (!cb || type >= BM_CONFIG_MAX_EVENT_TYPES || !id) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = bm_hal_critical_enter();

    bm_subscriber_t *sub = NULL;
    for (int i = 0; i < BM_CONFIG_MAX_EVENT_SUBSCRIBERS; i++) {
        if (_subscribers[i].id == 0) {
            sub = &_subscribers[i];
            break;
        }
    }
    if (!sub) {
        bm_hal_critical_exit(s);
        return BM_ERR_NO_MEM;
    }

    sub->cb = cb;
    sub->user_data = user_data;
    sub->id = _next_subscriber_id++;
    sub->next = _event_types[type].head;
    _event_types[type].head = sub;
    *id = sub->id;

    bm_hal_critical_exit(s);
    return BM_OK;
}

int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES || id == 0) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = bm_hal_critical_enter();

    bm_subscriber_t **pp = &_event_types[type].head;
    while (*pp) {
        if ((*pp)->id == id) {
            bm_subscriber_t *to_remove = *pp;
            *pp = to_remove->next;
            to_remove->id = 0;
            to_remove->cb = NULL;
            to_remove->next = NULL;
            bm_hal_critical_exit(s);
            return BM_OK;
        }
        pp = &(*pp)->next;
    }

    bm_hal_critical_exit(s);
    return BM_ERR_NOT_FOUND;
}
