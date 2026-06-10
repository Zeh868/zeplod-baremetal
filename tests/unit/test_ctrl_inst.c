#include "unity.h"
#include "bm_ctrl_inst.h"
#include "bm_hrt.h"
#include "bm_hal_adc_sim.h"
#include "bm_hal_pwm_sim.h"
#include "bm_hal_timer_native.h"

static uint32_t g_step_count;
static uint32_t g_hw_step_count;

static int mock_init(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static int mock_start(const bm_ctrl_inst_t *instance) {
    (void)instance;
    return BM_OK;
}

static void mock_safe_stop(const bm_ctrl_inst_t *instance) {
    (void)instance;
}

static void scheduled_step(const bm_ctrl_inst_t *instance) {
    (void)instance;
    g_step_count++;
}

static void hardware_step(const bm_ctrl_inst_t *instance) {
    (void)instance;
    g_hw_step_count++;
}

static int bind_adc(const bm_ctrl_inst_t *instance,
                    const bm_hal_hrt_binding_t *binding) {
    static const bm_hal_adc_t *adc;
    (void)instance;
    adc = &BM_HAL_ADC_SIM0;
    return bm_hal_adc_bind_complete(adc, binding);
}

static const bm_ctrl_ops_t mock_ops = {
    mock_init,
    mock_start,
    mock_safe_stop
};

static const bm_ctrl_slot_t scheduled_slots[] = {
    {
        BM_CTRL_SLOT_SCHEDULED,
        1000u,
        BM_HRT_TRIGGER_TIMER,
        scheduled_step,
        NULL,
        "sched"
    },
};

static const bm_ctrl_slot_t hardware_slots[] = {
    {
        BM_CTRL_SLOT_HARDWARE,
        0u,
        BM_HRT_TRIGGER_ADC_COMPLETE,
        hardware_step,
        bind_adc,
        "hw"
    },
};

static const bm_ctrl_inst_t sched_inst = {
    1u, "sched", NULL, NULL, NULL,
    scheduled_slots,
    (uint32_t)(sizeof(scheduled_slots) / sizeof(scheduled_slots[0])),
    NULL, 0u, &mock_ops
};

static const bm_ctrl_inst_t hw_inst = {
    2u, "hw", NULL, NULL, NULL,
    hardware_slots,
    (uint32_t)(sizeof(hardware_slots) / sizeof(hardware_slots[0])),
    NULL, 0u, &mock_ops
};

void setUp(void) {
    g_step_count = 0u;
    g_hw_step_count = 0u;
    bm_hal_timer_native_reset_ticks();
    bm_hrt_reset();
}

void tearDown(void) {
    bm_ctrl_safe_stop_all(NULL, 0u);
}

void test_ctrl_init_and_scheduled_slot(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst };

    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 1u));
    TEST_ASSERT_EQUAL(BM_OK, bm_hrt_start());

    bm_hal_timer_native_advance_ticks(10u);
    TEST_ASSERT_GREATER_THAN(0u, g_step_count);

    bm_ctrl_safe_stop_all(instances, 1u);
}

void test_ctrl_hardware_bind_fires_step(void) {
    const bm_ctrl_inst_t *const instances[] = { &hw_inst };

    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 1u));
    bm_hal_adc_sim_fire_complete(&BM_HAL_ADC_SIM0);
    TEST_ASSERT_EQUAL(1u, g_hw_step_count);

    bm_ctrl_safe_stop_all(instances, 1u);
}

void test_ctrl_find_instance(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst, &hw_inst };

    TEST_ASSERT_NOT_NULL(bm_ctrl_find(instances, 2u, 2u));
    TEST_ASSERT_NULL(bm_ctrl_find(instances, 2u, 99u));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ctrl_init_and_scheduled_slot);
    RUN_TEST(test_ctrl_hardware_bind_fires_step);
    RUN_TEST(test_ctrl_find_instance);
    return UNITY_END();
}
