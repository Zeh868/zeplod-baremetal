#ifndef BM_CORE_H
#define BM_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BM_OK                0
#define BM_ERR_NO_MEM       -1
#define BM_ERR_NOT_FOUND    -2
#define BM_ERR_WOULD_BLOCK  -3
#define BM_ERR_INVALID      -4
#define BM_ERR_BUSY         -5
#define BM_ERR_OVERFLOW     -6
#define BM_ERR_NOT_INIT     -7
#define BM_ERR_ALREADY      -8

typedef uint8_t bm_event_type_t;
typedef uint8_t bm_event_priority_t;
typedef uint32_t bm_event_subscriber_id_t;
typedef uint32_t bm_irq_state_t;
typedef volatile uint32_t bm_atomic_t;

typedef struct {
    bm_event_type_t      type;
    bm_event_priority_t  priority;
    const void          *data;
    size_t               data_len;
    uint8_t              source_id;
} bm_event_t;

typedef void (*bm_event_callback_t)(const bm_event_t *event, void *user_data);

#endif /* BM_CORE_H */
