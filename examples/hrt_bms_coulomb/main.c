#include "bm_ctrl_inst.h"
#include "bm_hrt.h"
#include "bm_snapshot.h"
#include "bm_ticker.h"
#include "hybrid_print.h"

#include "bm_hal_adc_sim.h"
#include "bm_hal_comp_sim.h"
#include "bm_event.h"
#include "bm_hal_pwm_sim.h"
#include "bm_hal_timer.h"
#include "bm_hal_uart.h"

#ifdef NATIVE_SIM
#include "bm_hal_timer_native.h"
#endif

#include <stdint.h>

#define TICKER_INTEGRATE 1u

BM_SNAPSHOT_DEFINE(pack_current, float);

static pack_current_snapshot_t g_pack_current = BM_SNAPSHOT_INITIALIZER;

typedef struct {
    uint32_t sample_count;
} pack_state_t;

typedef struct {
    float coulomb_ah;
    uint32_t integrate_ticks;
} cell_state_t;

static pack_state_t g_pack_state;
static cell_state_t g_cell_state;

static void pack_sample_step(const bm_ctrl_inst_t *instance) {
    pack_state_t *state = (pack_state_t *)instance->state;
    uint16_t raw = 0u;
    float amps;

    if (bm_hal_adc_read_injected(&BM_HAL_ADC_SIM0, 0u, &raw) != BM_OK) {
        return;
    }

    amps = (float)raw / 4096.0f;
    BM_SNAPSHOT_PUBLISH(g_pack_current, &amps);
    state->sample_count++;
}

static int bind_pack_adc(const bm_ctrl_inst_t *instance,
                         const bm_hal_hrt_binding_t *binding) {
    (void)instance;
    return bm_hal_adc_bind_complete(&BM_HAL_ADC_SIM0, binding);
}

static int pack_init(const bm_ctrl_inst_t *instance) {
    (void)instance;
    bm_hal_adc_sim_set_rank(&BM_HAL_ADC_SIM0, 0u, 4095u);
    (void)bm_hal_pwm_enable_outputs(&BM_HAL_PWM_SIM0, 1);
    return BM_OK;
}

static int pack_start(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static void pack_safe_stop(const bm_ctrl_inst_t *instance) {
    (void)instance;
    bm_hal_pwm_request_safe_state(&BM_HAL_PWM_SIM0);
}

static const bm_ctrl_ops_t g_pack_ops = {
    pack_init, pack_start, pack_safe_stop
};

static const bm_ctrl_slot_t g_pack_slots[] = {
    {
        BM_CTRL_SLOT_HARDWARE,
        0u,
        BM_HRT_TRIGGER_ADC_COMPLETE,
        pack_sample_step,
        bind_pack_adc,
        "pack_sample"
    }
};

static const bm_ctrl_inst_t g_pack_sampler = {
    10u, "pack_sampler", &g_pack_state, NULL, NULL,
    g_pack_slots, 1u, NULL, 0u, &g_pack_ops
};

static void cell_integrate_step(const bm_ctrl_inst_t *instance) {
    cell_state_t *state = (cell_state_t *)instance->state;
    float amps;

    BM_SNAPSHOT_READ(g_pack_current, &amps);
    state->coulomb_ah += amps / 3600.0f;
    state->integrate_ticks++;
}

static int cell_init(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static int cell_start(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static void cell_safe_stop(const bm_ctrl_inst_t *instance) {
    (void)instance;
}

static const bm_ctrl_ops_t g_cell_ops = {
    cell_init, cell_start, cell_safe_stop
};

static const bm_ticker_slot_t g_cell_ticker[] = {
    { 100u, TICKER_INTEGRATE, 1u, "integrate" }
};

static const bm_ctrl_inst_t g_cell_0 = {
    20u, "cell_0", &g_cell_state, NULL, NULL,
    NULL, 0u, NULL, 0u, &g_cell_ops
};

static const bm_ctrl_inst_t *const g_instances[] = {
    &g_pack_sampler,
    &g_cell_0
};

static void on_integrate(const bm_event_t *event, void *user_data) {
    (void)user_data;
    if (event->type == TICKER_INTEGRATE) {
        cell_integrate_step(&g_cell_0);
    }
}

static void run_sim(uint32_t cycles) {
    uint32_t i;

    for (i = 0u; i < cycles; ++i) {
#ifdef NATIVE_SIM
        bm_hal_timer_native_advance_ticks(1u);
#else
        for (volatile uint32_t d = 0u; d < 20u; ++d) {
        }
#endif
        if ((i % 1u) == 0u) {
            bm_hal_adc_sim_fire_complete(&BM_HAL_ADC_SIM0);
        }
        (void)bm_ticker_poll();
        (void)bm_event_process(4u);
    }
}

int main(void) {
    bm_event_subscriber_id_t sub;
    int rc;

    bm_hal_uart_init(NULL);
    bm_event_reset();
    bm_event_register_type(TICKER_INTEGRATE, "INTEGRATE");
    bm_event_subscribe(TICKER_INTEGRATE, on_integrate, NULL, &sub);

    (void)bm_hal_timer_init(1000u);
    bm_hal_comp_sim_set_threshold(&BM_HAL_COMP_SIM0, 3000u);

    rc = bm_ctrl_init_all(g_instances, 2u);
    if (rc != BM_OK) {
        hybrid_print("EXAMPLE_HRT_BMS_COULOMB: FAIL init\n");
        return 1;
    }
    rc = bm_hrt_start();
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_ctrl_start_all(g_instances, 2u);
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_ticker_init(g_cell_ticker, 1u);
    if (rc != BM_OK) {
        return 1;
    }

#ifdef NATIVE_SIM
    run_sim(1000u);
#else
    run_sim(500u);
#endif

    bm_hal_comp_sim_trip(&BM_HAL_COMP_SIM0, 4000u);
    bm_hal_pwm_request_safe_state(&BM_HAL_PWM_SIM0);

    if (g_pack_state.sample_count > 0u &&
        g_cell_state.integrate_ticks > 0u &&
        g_cell_state.coulomb_ah > 0.0001f &&
        !bm_hal_pwm_sim_outputs_enabled(&BM_HAL_PWM_SIM0)) {
        hybrid_print_pass("HRT_BMS_COULOMB");
    } else {
        hybrid_print("EXAMPLE_HRT_BMS_COULOMB: FAIL metrics\n");
        bm_ctrl_safe_stop_all(g_instances, 2u);
        return 1;
    }

    bm_ctrl_safe_stop_all(g_instances, 2u);
#ifdef NATIVE_SIM
    return 0;
#else
    while (1) {
    }
#endif
}
