#include "bm_ctrl_inst.h"
#include "bm_hrt.h"
#include "bm_sync.h"
#include "hybrid_print.h"

#include "bm_hal_pwm_sim.h"
#include "bm_hal_timer.h"
#include "bm_hal_uart.h"

#ifdef NATIVE_SIM
#include "bm_hal_timer_native.h"
#endif

#include <stdint.h>

struct bm_hal_timer {
    uint32_t id;
};

static const struct bm_hal_timer g_master_timer = { 0u };

typedef struct {
    uint32_t start_tick;
    int started;
} axis_state_t;

static axis_state_t g_axis_state[3];

static void axis_step(const bm_ctrl_inst_t *instance) {
    axis_state_t *state = (axis_state_t *)instance->state;
    if (!state->started) {
        state->start_tick = bm_hal_timer_get_ticks();
        state->started = 1;
        (void)bm_hal_pwm_enable_outputs(
            (const bm_hal_pwm_t *)instance->config, 1);
    }
}

static int axis_init(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static int axis_start(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static void axis_safe_stop(const bm_ctrl_inst_t *instance) {
    const bm_hal_pwm_t *pwm = (const bm_hal_pwm_t *)instance->config;
    if (pwm) {
        bm_hal_pwm_request_safe_state(pwm);
    }
}

static const bm_ctrl_ops_t g_axis_ops = {
    axis_init, axis_start, axis_safe_stop
};

static const bm_ctrl_slot_t g_axis_slot[] = {
    {
        BM_CTRL_SLOT_SCHEDULED,
        1000u,
        BM_HRT_TRIGGER_TIMER,
        axis_step,
        NULL,
        "axis"
    }
};

#define DEFINE_AXIS(ID, PWM, STATE) \
    static const bm_ctrl_inst_t axis_##ID = { \
        (ID), "axis_" #ID, (STATE), (const void *)(PWM), NULL, \
        g_axis_slot, 1u, NULL, 0u, &g_axis_ops \
    }

DEFINE_AXIS(1, &BM_HAL_PWM_SIM0, &g_axis_state[0]);
DEFINE_AXIS(2, &BM_HAL_PWM_SIM1, &g_axis_state[1]);
DEFINE_AXIS(3, &BM_HAL_PWM_SIM2, &g_axis_state[2]);

static const bm_ctrl_inst_t *const g_members[] = {
    &axis_1, &axis_2, &axis_3
};

static const uint32_t g_phase_ticks[] = { 0u, 0u, 0u };

static const bm_sync_domain_t g_domain = {
    "robot",
    &g_master_timer,
    g_members,
    g_phase_ticks,
    3u
};

static const bm_ctrl_inst_t *const g_instances[] = {
    &axis_1, &axis_2, &axis_3
};

int main(void) {
    uint32_t i;
    int rc;

    bm_hal_uart_init(NULL);
    (void)bm_hal_timer_init(10000u);

    rc = bm_ctrl_init_all(g_instances, 3u);
    if (rc != BM_OK) {
        hybrid_print("EXAMPLE_MULTI_AXIS_SYNC: FAIL init\n");
        return 1;
    }
    rc = bm_sync_configure(&g_domain);
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_sync_arm(&g_domain);
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_hrt_start();
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_ctrl_start_all(g_instances, 3u);
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_sync_trigger(&g_domain);
    if (rc != BM_OK) {
        return 1;
    }

#ifdef NATIVE_SIM
    for (i = 0u; i < 50u; ++i) {
        bm_hal_timer_native_advance_ticks(10u);
    }
#else
    for (i = 0u; i < 200u; ++i) {
        for (volatile uint32_t d = 0u; d < 20u; ++d) {
        }
    }
#endif

    if (g_axis_state[0].started && g_axis_state[1].started &&
        g_axis_state[2].started &&
        g_axis_state[0].start_tick == g_axis_state[1].start_tick &&
        g_axis_state[1].start_tick == g_axis_state[2].start_tick) {
        hybrid_print_pass("MULTI_AXIS_SYNC");
    } else {
        hybrid_print("EXAMPLE_MULTI_AXIS_SYNC: FAIL sync\n");
        bm_sync_safe_stop(&g_domain);
        bm_ctrl_safe_stop_all(g_instances, 3u);
        return 1;
    }

    bm_sync_safe_stop(&g_domain);
    bm_ctrl_safe_stop_all(g_instances, 3u);
#ifdef NATIVE_SIM
    return 0;
#else
    while (1) {
    }
#endif
}
