/**
 * @file test_sync.c
 * @brief 同步域 configure/arm/trigger 状态机单元测试
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "unity.h"
#include "bm_sync.h"
#include "bm_log.h"

/* 占位 HAL 定时器与成员，满足 bm_sync_domain_t 字段要求 */
struct bm_hal_timer {
    int placeholder;
};

static const struct bm_hal_timer dummy_timer_storage;
static const bm_ctrl_inst_t dummy_member;
static const bm_ctrl_inst_t *const members[] = { &dummy_member };
static const uint32_t phases[] = { 0u };

static bm_sync_domain_t domain;
static bm_sync_domain_t domain_b;

static void init_domain(void) {
    domain.name = "test";
    domain.master_timer = &dummy_timer_storage;
    domain.members = members;
    domain.phase_ticks = phases;
    domain.member_count = 1u;
}

void setUp(void) {
    BM_LOGI("test_sync", "setUp: init domain and safe_stop");
    init_domain();
    bm_sync_safe_stop(&domain);
}

void tearDown(void) {
    bm_sync_safe_stop(&domain);
}

void test_sync_configure_arm_trigger(void) {
    /* 正常路径：配置 → 武装 → 触发 */
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_arm(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_trigger(&domain));
}

void test_sync_safe_stop_null_clears_active(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain));
    bm_sync_safe_stop(NULL);
    TEST_ASSERT_EQUAL(BM_ERR_NOT_INIT, bm_sync_arm(&domain));
}

void test_sync_trigger_before_arm_fails(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain));
    BM_LOGE("test_sync", "expect NOT_INIT when trigger before arm");
    TEST_ASSERT_EQUAL(BM_ERR_NOT_INIT, bm_sync_trigger(&domain));
}

void test_sync_retrigger_requires_rearm(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_arm(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_trigger(&domain));
    TEST_ASSERT_EQUAL(BM_ERR_NOT_INIT, bm_sync_trigger(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_arm(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_trigger(&domain));
}

void test_sync_configure_switches_active_domain(void) {
    init_domain();
    domain_b = domain;
    domain_b.name = "test_b";

    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_arm(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain_b));
    TEST_ASSERT_EQUAL(BM_ERR_NOT_INIT, bm_sync_arm(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_arm(&domain_b));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sync_configure_arm_trigger);
    RUN_TEST(test_sync_trigger_before_arm_fails);
    RUN_TEST(test_sync_safe_stop_null_clears_active);
    RUN_TEST(test_sync_retrigger_requires_rearm);
    RUN_TEST(test_sync_configure_switches_active_domain);
    return UNITY_END();
}
