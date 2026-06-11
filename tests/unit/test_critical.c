/**
 * @file test_critical.c
 * @brief 原子变量饱和与空指针单元测试
 */
#include "unity.h"
#include "bm_atomic.h"

void setUp(void) {}
void tearDown(void) {}

void test_atomic_inc_saturates_at_max(void) {
    bm_atomic_t v = UINT32_MAX;

    TEST_ASSERT_EQUAL(UINT32_MAX, bm_atomic_inc(&v));
    TEST_ASSERT_EQUAL(UINT32_MAX, bm_atomic_load(&v));
}

void test_atomic_load_null_returns_zero(void) {
    TEST_ASSERT_EQUAL(0u, bm_atomic_load(NULL));
}

void test_atomic_inc_null_returns_zero(void) {
    TEST_ASSERT_EQUAL(0u, bm_atomic_inc(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_atomic_inc_saturates_at_max);
    RUN_TEST(test_atomic_load_null_returns_zero);
    RUN_TEST(test_atomic_inc_null_returns_zero);
    return UNITY_END();
}
