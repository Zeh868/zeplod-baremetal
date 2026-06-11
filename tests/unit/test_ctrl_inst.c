/**
 * @file test_ctrl_inst.c
 * @brief 控制器实例生命周期、HRT 槽绑定与失败回滚单元测试
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "unity.h"
#include "bm_ctrl_inst.h"
#include "bm_hrt.h"
#include "bm_hal_adc_sim.h"
#include "bm_hal_pwm_sim.h"
#include "bm_hal_timer_native.h"
#include "bm_log.h"

/* 各 mock 回调的调用计数 */
static uint32_t g_step_count;
static uint32_t g_hw_step_count;
static uint32_t g_safe_stop_count;
static uint32_t g_inst2_started;
static uint32_t g_inst2_premature_steps;

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
    g_safe_stop_count++;
}

static int init_fail_id2(const bm_ctrl_inst_t *instance) {
    if (instance->id == 2u) {
        return BM_ERR_INVALID;
    }
    return BM_OK;
}

static int start_fail_id2(const bm_ctrl_inst_t *instance) {
    if (instance->id == 2u) {
        return BM_ERR_INVALID;
    }
    return BM_OK;
}

static int bind_fail(const bm_ctrl_inst_t *instance,
                    const bm_hal_hrt_binding_t *binding) {
    (void)instance;
    (void)binding;
    return BM_ERR_INVALID;
}

static void scheduled_step(const bm_ctrl_inst_t *instance) {
    (void)instance;
    g_step_count++;
}

static void inst2_scheduled_step(const bm_ctrl_inst_t *instance) {
    if (instance->id == 2u && g_inst2_started == 0u) {
        g_inst2_premature_steps++;
    }
    g_step_count++;
}

static int inst1_start_advance_ticks(const bm_ctrl_inst_t *instance) {
    if (instance->id == 1u) {
        bm_hal_timer_native_advance_ticks(15u);
    }
    return BM_OK;
}

static int inst2_start_mark(const bm_ctrl_inst_t *instance) {
    if (instance->id == 2u) {
        g_inst2_started = 1u;
    }
    return BM_OK;
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

/* mock 控制器 ops：init/start/safe_stop */
static const bm_ctrl_ops_t mock_ops = {
    mock_init,
    mock_start,
    mock_safe_stop
};

static const bm_ctrl_ops_t inst1_advance_ops = {
    mock_init,
    inst1_start_advance_ticks,
    mock_safe_stop
};

static const bm_ctrl_ops_t inst2_mark_ops = {
    mock_init,
    inst2_start_mark,
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

/* 定时调度槽实例（1000us 周期） */
static const bm_ctrl_inst_t sched_inst = {
    1u, "sched", NULL, NULL, NULL,
    scheduled_slots,
    (uint32_t)(sizeof(scheduled_slots) / sizeof(scheduled_slots[0])),
    NULL, 0u, &mock_ops
};

static const bm_ctrl_slot_t inst2_scheduled_slots[] = {
    {
        BM_CTRL_SLOT_SCHEDULED,
        1000u,
        BM_HRT_TRIGGER_TIMER,
        inst2_scheduled_step,
        NULL,
        "sched2"
    },
};

static const bm_ctrl_inst_t sched_inst_advance = {
    1u, "sched_adv", NULL, NULL, NULL,
    scheduled_slots,
    (uint32_t)(sizeof(scheduled_slots) / sizeof(scheduled_slots[0])),
    NULL, 0u, &inst1_advance_ops
};

static const bm_ctrl_inst_t sched_inst2 = {
    2u, "sched2", NULL, NULL, NULL,
    inst2_scheduled_slots,
    (uint32_t)(sizeof(inst2_scheduled_slots) / sizeof(inst2_scheduled_slots[0])),
    NULL, 0u, &inst2_mark_ops
};

/* 硬件触发槽实例（ADC 完成中断） */
static const bm_ctrl_inst_t hw_inst = {
    2u, "hw", NULL, NULL, NULL,
    hardware_slots,
    (uint32_t)(sizeof(hardware_slots) / sizeof(hardware_slots[0])),
    NULL, 0u, &mock_ops
};

static const bm_ctrl_ops_t fail_init_ops = {
    init_fail_id2, mock_start, mock_safe_stop
};

static const bm_ctrl_ops_t fail_start_ops = {
    mock_init, start_fail_id2, mock_safe_stop
};

static const bm_ctrl_ops_t fail_bind_ops = {
    mock_init, mock_start, mock_safe_stop
};

static const bm_ctrl_slot_t bind_fail_slots[] = {
    {
        BM_CTRL_SLOT_HARDWARE,
        0u,
        BM_HRT_TRIGGER_ADC_COMPLETE,
        hardware_step,
        bind_fail,
        "bind_fail"
    },
};

static const bm_ctrl_inst_t fail_init_inst = {
    2u, "fail_init", NULL, NULL, NULL,
    scheduled_slots,
    (uint32_t)(sizeof(scheduled_slots) / sizeof(scheduled_slots[0])),
    NULL, 0u, &fail_init_ops
};

static const bm_ctrl_inst_t fail_start_inst = {
    2u, "fail_start", NULL, NULL, NULL,
    scheduled_slots,
    (uint32_t)(sizeof(scheduled_slots) / sizeof(scheduled_slots[0])),
    NULL, 0u, &fail_start_ops
};

static const bm_ctrl_inst_t bind_fail_inst = {
    3u, "bind_fail", NULL, NULL, NULL,
    bind_fail_slots,
    (uint32_t)(sizeof(bind_fail_slots) / sizeof(bind_fail_slots[0])),
    NULL, 0u, &fail_bind_ops
};

void setUp(void) {
    BM_LOGI("test_ctrl", "setUp: reset counters and HRT");
    g_step_count = 0u;
    g_hw_step_count = 0u;
    g_safe_stop_count = 0u;
    bm_hal_timer_native_reset_ticks();
    bm_hrt_reset();
}

void tearDown(void) {
    bm_ctrl_safe_stop_all(NULL, 0u);
    BM_LOGI("test_ctrl", "tearDown: safe_stop_all done");
}

void test_ctrl_init_and_scheduled_slot(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst };

    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 1u));
    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_start_all(instances, 1u));

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

void test_ctrl_hardware_only_hrt_start_ok(void) {
    const bm_ctrl_inst_t *const instances[] = { &hw_inst };

    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 1u));
    TEST_ASSERT_EQUAL(BM_OK, bm_hrt_start());
    bm_hal_adc_sim_fire_complete(&BM_HAL_ADC_SIM0);
    TEST_ASSERT_EQUAL(1u, g_hw_step_count);
    bm_ctrl_safe_stop_all(instances, 1u);
}

void test_ctrl_init_failure_rolls_back_safe_stop(void) {
    /* 第二实例 init 失败时，应回滚并调用已 init 实例的 safe_stop */
    const bm_ctrl_inst_t *const instances[] = { &sched_inst, &fail_init_inst };

    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_ctrl_init_all(instances, 2u));
    TEST_ASSERT_EQUAL(1u, g_safe_stop_count);
    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(&instances[0], 1u));
    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_start_all(&instances[0], 1u));
    bm_hal_timer_native_advance_ticks(10u);
    TEST_ASSERT_GREATER_THAN(0u, g_step_count);
    bm_ctrl_safe_stop_all(&instances[0], 1u);
}

void test_ctrl_start_failure_rolls_back_safe_stop(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst, &fail_start_inst };

    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 2u));
    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_ctrl_start_all(instances, 2u));
    TEST_ASSERT_EQUAL(2u, g_safe_stop_count);
}

void test_ctrl_start_all_starts_hrt_after_all_instances(void) {
    const bm_ctrl_inst_t *const instances[] = {
        &sched_inst_advance, &sched_inst2
    };

    g_inst2_started = 0u;
    g_inst2_premature_steps = 0u;
    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 2u));
    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_start_all(instances, 2u));
    TEST_ASSERT_EQUAL(0u, g_inst2_premature_steps);
    bm_ctrl_safe_stop_all(instances, 2u);
}

void test_ctrl_start_all_without_init_fails(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst };

    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_ctrl_start_all(instances, 1u));
}

void test_ctrl_init_rejects_null_instance(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst, NULL };

    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_ctrl_init_all(instances, 2u));
}

void test_ctrl_rejects_double_start(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst };

    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 1u));
    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_start_all(instances, 1u));
    TEST_ASSERT_EQUAL(BM_ERR_ALREADY, bm_ctrl_start_all(instances, 1u));
    bm_ctrl_safe_stop_all(instances, 1u);
}

void test_ctrl_bind_failure_rolls_back(void) {
    const bm_ctrl_inst_t *const instances[] = { &bind_fail_inst };

    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_ctrl_init_all(instances, 1u));
    bm_hal_adc_sim_fire_complete(&BM_HAL_ADC_SIM0);
    TEST_ASSERT_EQUAL(0u, g_hw_step_count);
}

void test_ctrl_find_instance(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst, &hw_inst };

    TEST_ASSERT_NOT_NULL(bm_ctrl_find(instances, 2u, 2u));
    TEST_ASSERT_NULL(bm_ctrl_find(instances, 2u, 99u));
}

void test_ctrl_reinit_teardowns_previous_session(void) {
    const bm_ctrl_inst_t *const instances[] = { &sched_inst };

    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 1u));
    g_safe_stop_count = 0u;
    TEST_ASSERT_EQUAL(BM_OK, bm_ctrl_init_all(instances, 1u));
    TEST_ASSERT_EQUAL(1u, g_safe_stop_count);
    bm_ctrl_safe_stop_all(instances, 1u);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ctrl_init_and_scheduled_slot);
    RUN_TEST(test_ctrl_hardware_bind_fires_step);
    RUN_TEST(test_ctrl_hardware_only_hrt_start_ok);
    RUN_TEST(test_ctrl_init_failure_rolls_back_safe_stop);
    RUN_TEST(test_ctrl_start_failure_rolls_back_safe_stop);
    RUN_TEST(test_ctrl_bind_failure_rolls_back);
    RUN_TEST(test_ctrl_find_instance);
    RUN_TEST(test_ctrl_reinit_teardowns_previous_session);
    RUN_TEST(test_ctrl_start_all_starts_hrt_after_all_instances);
    RUN_TEST(test_ctrl_start_all_without_init_fails);
    RUN_TEST(test_ctrl_init_rejects_null_instance);
    RUN_TEST(test_ctrl_rejects_double_start);
    return UNITY_END();
}
