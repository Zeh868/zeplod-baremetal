#ifndef BM_EVENT_H
#define BM_EVENT_H

#include "bm_types.h"

#include <stddef.h>

typedef uint8_t bm_event_type_t;
typedef uint8_t bm_event_priority_t;
typedef uint32_t bm_event_subscriber_id_t;

typedef struct {
    bm_event_type_t      type;
    bm_event_priority_t  priority;
    const void          *data;
    size_t               data_len;
    uint8_t              source_id;
} bm_event_t;

typedef void (*bm_event_callback_t)(const bm_event_t *event, void *user_data);

void bm_event_reset(void);
int bm_event_register_type(bm_event_type_t type, const char *name);
int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id);
int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id);
int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len);
int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len);
int bm_event_publish_event(const bm_event_t *event);
int bm_event_publish_event_from_isr(const bm_event_t *event);
int bm_event_process(uint32_t max_events);

#endif /* BM_EVENT_H */
