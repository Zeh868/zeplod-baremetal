#include "bm_ctrl_inst.h"
#include "bm_hrt.h"

#include <string.h>

#ifndef BM_CONFIG_HRT_TICK_US
#define BM_CONFIG_HRT_TICK_US 100u
#endif

#ifndef BM_CONFIG_HRT_MAX_SLOTS
#define BM_CONFIG_HRT_MAX_SLOTS 16u
#endif

#ifndef BM_CONFIG_MAX_CTRL_SLOTS
#define BM_CONFIG_MAX_CTRL_SLOTS 32u
#endif

#ifndef BM_CONFIG_MAX_CTRL_INSTANCES
#define BM_CONFIG_MAX_CTRL_INSTANCES 16u
#endif

typedef struct {
    const bm_ctrl_inst_t *instance;
    const bm_ctrl_slot_t *slot;
} bm_ctrl_binding_t;

static const bm_ctrl_inst_t *g_instances[BM_CONFIG_MAX_CTRL_INSTANCES];
static uint32_t g_instance_count;
static bm_ctrl_binding_t g_bindings[BM_CONFIG_MAX_CTRL_SLOTS];
static uint32_t g_binding_count;
static bm_hrt_slot_t g_hrt_slots[BM_CONFIG_HRT_MAX_SLOTS];
static uint32_t g_hrt_slot_count;
static uint32_t g_init_done_count;

static void bm_ctrl_run_binding(void *context) {
    const bm_ctrl_binding_t *binding = (const bm_ctrl_binding_t *)context;
    binding->slot->step(binding->instance);
}

static void ctrl_clear_runtime(void) {
    {
        uint32_t n;
        for (n = 0u; n < BM_CONFIG_MAX_CTRL_INSTANCES; ++n) {
            g_instances[n] = NULL;
        }
    }
    g_instance_count = 0u;
    memset(g_bindings, 0, sizeof(g_bindings));
    g_binding_count = 0u;
    memset(g_hrt_slots, 0, sizeof(g_hrt_slots));
    g_hrt_slot_count = 0u;
    g_init_done_count = 0u;
}

static void ctrl_unbind_all_hardware(void) {
    uint32_t i;
    uint32_t s;

    for (i = 0u; i < g_instance_count; ++i) {
        const bm_ctrl_inst_t *inst = g_instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_ctrl_slot_t *slot = &inst->slots[s];
            if (slot->kind == BM_CTRL_SLOT_HARDWARE && slot->bind_hardware) {
                (void)slot->bind_hardware(inst, NULL);
            }
        }
    }
}

static void ctrl_rollback_inits(void) {
    while (g_init_done_count > 0u) {
        g_init_done_count--;
        const bm_ctrl_inst_t *inst = g_instances[g_init_done_count];
        if (inst->ops && inst->ops->safe_stop) {
            inst->ops->safe_stop(inst);
        }
    }
}

static int validate_instance(const bm_ctrl_inst_t *inst) {
    uint32_t s;

    if (!inst || !inst->ops) {
        return BM_ERR_INVALID;
    }
    if (inst->slot_count > 0u && !inst->slots) {
        return BM_ERR_INVALID;
    }
    if (inst->slot_count == 0u && inst->slots != NULL) {
        return BM_ERR_INVALID;
    }
    if (!inst->ops->init || !inst->ops->start || !inst->ops->safe_stop) {
        return BM_ERR_INVALID;
    }

    for (s = 0u; s < inst->slot_count; ++s) {
        const bm_ctrl_slot_t *slot = &inst->slots[s];
        if (!slot->step) {
            return BM_ERR_INVALID;
        }
        if (slot->kind == BM_CTRL_SLOT_SCHEDULED) {
            if (slot->bind_hardware != NULL) {
                return BM_ERR_INVALID;
            }
            if (slot->trigger != BM_HRT_TRIGGER_TIMER) {
                return BM_ERR_INVALID;
            }
            if (slot->period_us == 0u ||
                (slot->period_us % BM_CONFIG_HRT_TICK_US) != 0u) {
                return BM_ERR_INVALID;
            }
        } else if (slot->kind == BM_CTRL_SLOT_HARDWARE) {
            if (!slot->bind_hardware) {
                return BM_ERR_INVALID;
            }
        } else {
            return BM_ERR_INVALID;
        }
    }

    return BM_OK;
}

static int validate_unique_ids(const bm_ctrl_inst_t *const *instances,
                               uint32_t count) {
    uint32_t i;
    uint32_t j;

    for (i = 0u; i < count; ++i) {
        for (j = i + 1u; j < count; ++j) {
            if (instances[i]->id == instances[j]->id) {
                return BM_ERR_ALREADY;
            }
        }
    }
    return BM_OK;
}

static int assemble_tables(const bm_ctrl_inst_t *const *instances,
                           uint32_t count) {
    uint32_t i;
    uint32_t s;

    g_binding_count = 0u;
    g_hrt_slot_count = 0u;

    for (i = 0u; i < count; ++i) {
        const bm_ctrl_inst_t *inst = instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_ctrl_slot_t *slot = &inst->slots[s];

            if (g_binding_count >= BM_CONFIG_MAX_CTRL_SLOTS) {
                return BM_ERR_OVERFLOW;
            }
            g_bindings[g_binding_count].instance = inst;
            g_bindings[g_binding_count].slot = slot;
            g_binding_count++;

            if (slot->kind != BM_CTRL_SLOT_SCHEDULED) {
                continue;
            }
            if (g_hrt_slot_count >= BM_CONFIG_HRT_MAX_SLOTS) {
                return BM_ERR_OVERFLOW;
            }
            g_hrt_slots[g_hrt_slot_count].period_us = slot->period_us;
            g_hrt_slots[g_hrt_slot_count].trigger = BM_HRT_TRIGGER_TIMER;
            g_hrt_slots[g_hrt_slot_count].callback = bm_ctrl_run_binding;
            g_hrt_slots[g_hrt_slot_count].context =
                &g_bindings[g_binding_count - 1u];
            g_hrt_slots[g_hrt_slot_count].name = slot->name;
            g_hrt_slot_count++;
        }
    }

    return BM_OK;
}

int bm_ctrl_init_all(const bm_ctrl_inst_t *const *instances, uint32_t count) {
    const bm_resource_claim_t *claim_ptrs[BM_CONFIG_MAX_CTRL_INSTANCES];
    uint32_t claim_counts[BM_CONFIG_MAX_CTRL_INSTANCES];
    uint32_t i;
    uint32_t s;
    int rc;

    ctrl_clear_runtime();

    if (!instances || count == 0u || count > BM_CONFIG_MAX_CTRL_INSTANCES) {
        return BM_ERR_INVALID;
    }

    rc = validate_unique_ids(instances, count);
    if (rc != BM_OK) {
        return rc;
    }

    for (i = 0u; i < count; ++i) {
        rc = validate_instance(instances[i]);
        if (rc != BM_OK) {
            return rc;
        }
        claim_ptrs[i] = instances[i]->claims;
        claim_counts[i] = instances[i]->claim_count;
    }

    rc = bm_resource_check_conflicts(claim_ptrs, claim_counts, count);
    if (rc != BM_OK) {
        return rc;
    }

    for (i = 0u; i < count; ++i) {
        g_instances[i] = instances[i];
    }
    g_instance_count = count;

    rc = assemble_tables(instances, count);
    if (rc != BM_OK) {
        ctrl_clear_runtime();
        return rc;
    }

    if (g_hrt_slot_count > 0u) {
        rc = bm_hrt_init(g_hrt_slots, g_hrt_slot_count);
        if (rc != BM_OK) {
            ctrl_clear_runtime();
            return rc;
        }
    }

    for (i = 0u; i < count; ++i) {
        rc = instances[i]->ops->init(instances[i]);
        if (rc != BM_OK) {
            ctrl_rollback_inits();
            bm_hrt_stop();
            ctrl_unbind_all_hardware();
            bm_hrt_reset();
            ctrl_clear_runtime();
            return rc;
        }
        g_init_done_count++;
    }

    for (i = 0u; i < count; ++i) {
        const bm_ctrl_inst_t *inst = instances[i];
        for (s = 0u; s < inst->slot_count; ++s) {
            const bm_ctrl_slot_t *slot = &inst->slots[s];
            bm_hal_hrt_binding_t hal_binding;
            const bm_ctrl_binding_t *binding = NULL;
            uint32_t b;

            if (slot->kind != BM_CTRL_SLOT_HARDWARE) {
                continue;
            }

            hal_binding.callback = bm_ctrl_run_binding;
            for (b = 0u; b < g_binding_count; ++b) {
                if (g_bindings[b].instance == inst &&
                    g_bindings[b].slot == slot) {
                    binding = &g_bindings[b];
                    break;
                }
            }
            if (!binding) {
                rc = BM_ERR_INVALID;
            } else {
                hal_binding.context = (void *)binding;
                rc = slot->bind_hardware(inst, &hal_binding);
            }
            if (rc != BM_OK) {
                ctrl_rollback_inits();
                bm_hrt_stop();
                ctrl_unbind_all_hardware();
                bm_hrt_reset();
                ctrl_clear_runtime();
                return rc;
            }
        }
    }

    return BM_OK;
}

int bm_ctrl_start_all(const bm_ctrl_inst_t *const *instances, uint32_t count) {
    uint32_t i;
    int rc;

    if (!instances || count == 0u || count != g_instance_count) {
        return BM_ERR_INVALID;
    }

    for (i = 0u; i < count; ++i) {
        if (instances[i] != g_instances[i]) {
            return BM_ERR_INVALID;
        }
        rc = instances[i]->ops->start(instances[i]);
        if (rc != BM_OK) {
            while (i > 0u) {
                --i;
                instances[i]->ops->safe_stop(instances[i]);
            }
            return rc;
        }
    }

    return BM_OK;
}

void bm_ctrl_safe_stop_all(const bm_ctrl_inst_t *const *instances,
                           uint32_t count) {
    uint32_t i;

    bm_hrt_stop();

    if (instances && count > 0u) {
        for (i = count; i > 0u; --i) {
            const bm_ctrl_inst_t *inst = instances[i - 1u];
            if (inst && inst->ops && inst->ops->safe_stop) {
                inst->ops->safe_stop(inst);
            }
        }
    } else if (g_instance_count > 0u) {
        for (i = g_instance_count; i > 0u; --i) {
            const bm_ctrl_inst_t *inst = g_instances[i - 1u];
            if (inst && inst->ops && inst->ops->safe_stop) {
                inst->ops->safe_stop(inst);
            }
        }
    }

    ctrl_unbind_all_hardware();
    bm_hrt_reset();
    ctrl_clear_runtime();
}

const bm_ctrl_inst_t *bm_ctrl_find(const bm_ctrl_inst_t *const *instances,
                                   uint32_t count,
                                   uint32_t id) {
    uint32_t i;

    if (!instances) {
        return NULL;
    }

    for (i = 0u; i < count; ++i) {
        if (instances[i] && instances[i]->id == id) {
            return instances[i];
        }
    }

    return NULL;
}
