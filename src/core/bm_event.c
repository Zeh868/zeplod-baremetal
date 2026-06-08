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
