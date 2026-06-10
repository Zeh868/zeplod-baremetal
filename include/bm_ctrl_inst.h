#ifndef BM_CTRL_INST_H
#define BM_CTRL_INST_H

#include "bm_hal_pwm.h"
#include "bm_hrt.h"
#include "bm_resource.h"
#include "bm_types.h"

typedef struct bm_ctrl_inst bm_ctrl_inst_t;

typedef void (*bm_ctrl_step_fn_t)(const bm_ctrl_inst_t *instance);

typedef enum {
    BM_CTRL_SLOT_HARDWARE,
    BM_CTRL_SLOT_SCHEDULED
} bm_ctrl_slot_kind_t;

typedef struct {
    bm_ctrl_slot_kind_t kind;
    uint32_t period_us;
    bm_hrt_trigger_t trigger;
    bm_ctrl_step_fn_t step;
    int (*bind_hardware)(const bm_ctrl_inst_t *instance,
                         const bm_hal_hrt_binding_t *binding);
    const char *name;
} bm_ctrl_slot_t;

typedef struct {
    int (*init)(const bm_ctrl_inst_t *instance);
    int (*start)(const bm_ctrl_inst_t *instance);
    void (*safe_stop)(const bm_ctrl_inst_t *instance);
} bm_ctrl_ops_t;

struct bm_ctrl_inst {
    uint32_t id;
    const char *name;
    void *state;
    const void *config;
    const void *resources;
    const bm_ctrl_slot_t *slots;
    uint32_t slot_count;
    const bm_resource_claim_t *claims;
    uint32_t claim_count;
    const bm_ctrl_ops_t *ops;
};

int bm_ctrl_init_all(const bm_ctrl_inst_t *const *instances, uint32_t count);
int bm_ctrl_start_all(const bm_ctrl_inst_t *const *instances, uint32_t count);
void bm_ctrl_safe_stop_all(const bm_ctrl_inst_t *const *instances,
                           uint32_t count);
const bm_ctrl_inst_t *bm_ctrl_find(const bm_ctrl_inst_t *const *instances,
                                   uint32_t count,
                                   uint32_t id);

#endif /* BM_CTRL_INST_H */
