/**
 * @file bm_exec.h
 * @brief Deterministic execution instances and batch lifecycle management.
 *
 * An execution instance owns state, configuration, resource claims, lifecycle
 * operations, and one or more deterministic execution slots. The core only
 * distinguishes hardware-event and periodic slots; device-specific trigger
 * semantics remain in the HAL binding adapter.
 */
#ifndef BM_EXEC_H
#define BM_EXEC_H

#include "hal/bm_hal_hrt.h"
#include "bm/hybrid/bm_resource.h"
#include "bm/common/bm_types.h"

typedef struct bm_exec bm_exec_t;

typedef enum {
    BM_EXEC_SESSION_NONE = 0,
    BM_EXEC_SESSION_INITED,
    BM_EXEC_SESSION_STARTED,
    BM_EXEC_SESSION_STOPPING
} bm_exec_session_t;

typedef void (*bm_exec_run_fn_t)(const bm_exec_t *instance);

typedef enum {
    BM_EXEC_SLOT_HARDWARE,
    BM_EXEC_SLOT_PERIODIC
} bm_exec_slot_kind_t;

typedef struct {
    bm_exec_slot_kind_t kind;
    uint32_t period_us;
    bm_exec_run_fn_t run;
    int (*bind)(const bm_exec_t *instance,
                const bm_hal_hrt_binding_t *binding);
    const char *name;
} bm_exec_slot_t;

typedef struct {
    int (*init)(const bm_exec_t *instance);
    int (*start)(const bm_exec_t *instance);
    void (*safe_stop)(const bm_exec_t *instance);
} bm_exec_ops_t;

struct bm_exec {
    uint32_t id;
    const char *name;
    void *state;
    const void *config;
    const void *resources;
    const bm_exec_slot_t *slots;
    uint32_t slot_count;
    const bm_resource_claim_t *claims;
    uint32_t claim_count;
    const bm_exec_ops_t *ops;
};

int bm_exec_init_all(const bm_exec_t *const *instances, uint32_t count);
int bm_exec_start_all(const bm_exec_t *const *instances, uint32_t count);
void bm_exec_safe_stop_all(const bm_exec_t *const *instances, uint32_t count);

const bm_exec_t *bm_exec_find(const bm_exec_t *const *instances,
                              uint32_t count,
                              uint32_t id);

bm_exec_session_t bm_exec_get_session(void);

#endif /* BM_EXEC_H */
