/**
 * @file test_wdg.c
 * @brief 软件看门狗模块超时与空指针单元测试
 */
#include "unity.h"
#include "bm_wdg.h"
#include "bm_hal_timer_native.h"
#include "bm_log.h"

void setUp(void) {
    bm_wdg_reset();
    bm_hal_timer_native_reset_ticks();
    (void)bm_hal_timer_init(1000u);
}

void tearDown(void) {}

void test_wdg_register_rejects_null(void) {
    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_wdg_register(NULL));
}

void test_wdg_feed_module_null_safe(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("mod_a"));
    bm_wdg_feed_module(NULL);
    bm_wdg_feed();
}

void test_wdg_blocks_hw_feed_until_module_fed(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_wdg_register("alive"));
    bm_wdg_feed();
    bm_wdg_feed_module("alive");
    bm_wdg_feed();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_wdg_register_rejects_null);
    RUN_TEST(test_wdg_feed_module_null_safe);
    RUN_TEST(test_wdg_blocks_hw_feed_until_module_fed);
    return UNITY_END();
}
