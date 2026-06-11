/**
 * @file test_wdg.c
 * @brief 软件看门狗模块超时与空指针单元测试
 */
#include "unity.h"
#include "bm_wdg.h"
#include "bm_hal_timer_native.h"
#include "bm_hal_wdg_native.h"
#include "bm_log.h"

void setUp(void) {
    bm_wdg_reset();
    bm_hal_timer_native_reset_ticks();
    bm_hal_wdg_native_reset_feed_count();
    (void)bm_hal_timer_init(1000u);
}

void tearDown(void) {}

void test_wdg_register_rejects_null(void) {
    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_wdg_register(NULL));
}

void test_wdg_register_rejects_duplicate(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("mod_a"));
    TEST_ASSERT_EQUAL(BM_ERR_ALREADY, bm_wdg_register("mod_a"));
}

void test_wdg_feed_module_null_safe(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("mod_a"));
    bm_wdg_feed_module(NULL);
    bm_wdg_feed();
    TEST_ASSERT_EQUAL(0u, bm_hal_wdg_native_get_feed_count());
}

void test_wdg_blocks_hw_feed_until_module_fed(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("alive"));

    bm_wdg_feed();
    TEST_ASSERT_EQUAL(0u, bm_hal_wdg_native_get_feed_count());

    bm_wdg_feed_module("alive");
    bm_wdg_feed();
    TEST_ASSERT_EQUAL(1u, bm_hal_wdg_native_get_feed_count());
}

void test_wdg_feed_at_tick_zero(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("zero_tick"));

    bm_hal_timer_native_reset_ticks();
    bm_wdg_feed_module("zero_tick");
    bm_wdg_feed();

    TEST_ASSERT_EQUAL(1u, bm_hal_wdg_native_get_feed_count());
}

void test_wdg_feed_expired_module_blocks_hw_feed(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("stale"));

    bm_hal_timer_native_reset_ticks();
    bm_wdg_feed_module("stale");
    bm_hal_timer_native_advance_ticks(1001u);
    bm_wdg_feed();

    TEST_ASSERT_EQUAL(0u, bm_hal_wdg_native_get_feed_count());
}

void test_wdg_feed_blocks_when_timer_not_ready(void) {
    bm_wdg_reset();
    bm_hal_timer_native_deinit();
    bm_hal_wdg_native_reset_feed_count();

    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("mod"));
    bm_wdg_feed_module("mod");
    bm_wdg_feed();

    TEST_ASSERT_EQUAL(0u, bm_hal_wdg_native_get_feed_count());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_wdg_register_rejects_null);
    RUN_TEST(test_wdg_register_rejects_duplicate);
    RUN_TEST(test_wdg_feed_module_null_safe);
    RUN_TEST(test_wdg_blocks_hw_feed_until_module_fed);
    RUN_TEST(test_wdg_feed_at_tick_zero);
    RUN_TEST(test_wdg_feed_expired_module_blocks_hw_feed);
    RUN_TEST(test_wdg_feed_blocks_when_timer_not_ready);
    return UNITY_END();
}
