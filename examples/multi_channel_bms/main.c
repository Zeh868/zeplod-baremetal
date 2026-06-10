#include "bm_ctrl_inst.h"
#include "bm_event.h"
#include "bm_hrt.h"
#include "bm_snapshot.h"
#include "bm_ticker.h"
#include "hybrid_print.h"

#include "bm_hal_adc_sim.h"
#include "bm_hal_timer.h"
#include "bm_hal_uart.h"

#ifdef NATIVE_SIM
#include "bm_hal_timer_native.h"
#endif

#include <stdint.h>

#define CELL_COUNT          16u
#define EVENT_CELL_CHECK    2u
#define PACK_ADC_KEY        100u
#define CELL_GPIO_BASE      200u

BM_SNAPSHOT_DEFINE(cell_voltage, uint16_t);

static cell_voltage_snapshot_t g_cell_voltages[CELL_COUNT];

typedef struct {
    uint32_t samples;
} pack_state_t;

typedef struct {
    uint32_t channel;
    uint32_t overvoltage_hits;
    uint16_t last_mv;
} cell_state_t;

static pack_state_t g_pack_state;
static cell_state_t g_cell_states[CELL_COUNT];

static void pack_sample_step(const bm_ctrl_inst_t *instance) {
    pack_state_t *state = (pack_state_t *)instance->state;
    uint32_t ch;
    uint16_t value;

    for (ch = 0u; ch < CELL_COUNT; ++ch) {
        value = (uint16_t)(3300u + ch);
        bm_hal_adc_sim_set_rank(&BM_HAL_ADC_SIM0, ch, value);
        if (bm_hal_adc_read_injected(&BM_HAL_ADC_SIM0, ch, &value) == BM_OK) {
            BM_SNAPSHOT_PUBLISH(g_cell_voltages[ch], &value);
        }
    }
    state->samples++;
}

static int bind_pack_adc(const bm_ctrl_inst_t *instance,
                         const bm_hal_hrt_binding_t *binding) {
    (void)instance;
    return bm_hal_adc_bind_complete(&BM_HAL_ADC_SIM0, binding);
}

static int pack_init(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static int pack_start(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static void pack_safe_stop(const bm_ctrl_inst_t *instance) {
    (void)instance;
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
        "pack"
    }
};

static const bm_resource_claim_t g_pack_claim_table[] = {
    { BM_RESOURCE_ADC_GROUP, PACK_ADC_KEY, BM_RESOURCE_OWNER, 1u, "adc_pack" }
};

static const bm_ctrl_inst_t g_pack_sampler = {
    1u, "pack_sampler", &g_pack_state, NULL, NULL,
    g_pack_slots, 1u, g_pack_claim_table, 1u, &g_pack_ops
};

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

static bm_resource_claim_t g_cell_claims[CELL_COUNT];
static bm_ctrl_inst_t g_cells[CELL_COUNT];
static const bm_ctrl_inst_t *g_instance_table[CELL_COUNT + 1u];

static const bm_ticker_slot_t g_check_ticker[] = {
    { 100u, EVENT_CELL_CHECK, 1u, "cell_check" }
};

static void on_cell_check(const bm_event_t *event, void *user_data) {
    uint32_t i;

    (void)user_data;
    if (event->type != EVENT_CELL_CHECK) {
        return;
    }

    for (i = 0u; i < CELL_COUNT; ++i) {
        uint16_t mv = 0u;
        BM_SNAPSHOT_READ(g_cell_voltages[i], &mv);
        g_cell_states[i].last_mv = mv;
        if (mv > 4200u) {
            g_cell_states[i].overvoltage_hits++;
        }
    }
}

static void setup_cells(void) {
    uint32_t i;

    for (i = 0u; i < CELL_COUNT; ++i) {
        g_cell_claims[i].kind = BM_RESOURCE_GPIO;
        g_cell_claims[i].key = CELL_GPIO_BASE + i;
        g_cell_claims[i].access = BM_RESOURCE_EXCLUSIVE;
        g_cell_claims[i].share_group = 0u;
        g_cell_claims[i].name = "cell_gpio";

        g_cell_states[i].channel = i;
        g_cells[i].id = 100u + i;
        g_cells[i].name = "cell";
        g_cells[i].state = &g_cell_states[i];
        g_cells[i].config = NULL;
        g_cells[i].resources = NULL;
        g_cells[i].slots = NULL;
        g_cells[i].slot_count = 0u;
        g_cells[i].claims = &g_cell_claims[i];
        g_cells[i].claim_count = 1u;
        g_cells[i].ops = &g_cell_ops;

        g_instance_table[i + 1u] = &g_cells[i];
    }
    g_instance_table[0] = &g_pack_sampler;
}

int main(void) {
    bm_event_subscriber_id_t sub;
    uint32_t i;
    uint32_t reported = 0u;
    int rc;

    bm_hal_uart_init(NULL);
    bm_event_reset();
    setup_cells();

    bm_event_register_type(EVENT_CELL_CHECK, "CELL_CHECK");
    bm_event_subscribe(EVENT_CELL_CHECK, on_cell_check, NULL, &sub);

    (void)bm_hal_timer_init(1000u);

    rc = bm_ctrl_init_all(g_instance_table, CELL_COUNT + 1u);
    if (rc != BM_OK) {
        hybrid_print("EXAMPLE_MULTI_CHANNEL_BMS: FAIL init\n");
        return 1;
    }
    rc = bm_hrt_start();
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_ctrl_start_all(g_instance_table, CELL_COUNT + 1u);
    if (rc != BM_OK) {
        return 1;
    }
    rc = bm_ticker_init(g_check_ticker, 1u);
    if (rc != BM_OK) {
        return 1;
    }

#ifdef NATIVE_SIM
    for (i = 0u; i < 500u; ++i) {
        bm_hal_timer_native_advance_ticks(1u);
        bm_hal_adc_sim_fire_complete(&BM_HAL_ADC_SIM0);
        (void)bm_ticker_poll();
        (void)bm_event_process(8u);
    }
#else
    for (i = 0u; i < 200u; ++i) {
        bm_hal_adc_sim_fire_complete(&BM_HAL_ADC_SIM0);
        (void)bm_ticker_poll();
        (void)bm_event_process(8u);
    }
#endif

    for (i = 0u; i < CELL_COUNT; ++i) {
        if (g_cell_states[i].last_mv >= 3300u) {
            reported++;
        }
    }

    if (g_pack_state.samples > 0u && reported == CELL_COUNT) {
        hybrid_print_pass("MULTI_CHANNEL_BMS");
    } else {
        hybrid_print("EXAMPLE_MULTI_CHANNEL_BMS: FAIL metrics\n");
        bm_ctrl_safe_stop_all(g_instance_table, CELL_COUNT + 1u);
        return 1;
    }

    bm_ctrl_safe_stop_all(g_instance_table, CELL_COUNT + 1u);
#ifdef NATIVE_SIM
    return 0;
#else
    while (1) {
    }
#endif
}
