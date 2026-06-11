/**
 * @file test_module.c
 * @brief 模块表 init/start/stop/deinit 生命周期与优先级顺序单元测试
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "unity.h"
#include "bm_module.h"
#include "bm_log.h"

static int g_init_count = 0;
static int g_start_count = 0;
static int g_stop_count = 0;
static int g_deinit_count = 0;
static int g_fail_mod_b_init = 0;
static int g_fail_mod_b_start = 0;

int mod_a_init(void) { g_init_count++; return BM_OK; }
int mod_a_start(void) { g_start_count++; return BM_OK; }
int mod_a_stop(void) { g_stop_count++; return BM_OK; }
int mod_a_deinit(void) { g_deinit_count++; return BM_OK; }

int mod_b_init(void) {
    g_init_count++;
    return g_fail_mod_b_init ? BM_ERR_INVALID : BM_OK;
}
int mod_b_start(void) {
    g_start_count++;
    return g_fail_mod_b_start ? BM_ERR_INVALID : BM_OK;
}

/* Explicit module table (compatible with all toolchains) */
const bm_module_t _bm_module_table[] = {
    { .name = "mod_b", .priority = 5, .init = mod_b_init, .start = mod_b_start },
    { .name = "mod_a", .priority = 1, .init = mod_a_init, .start = mod_a_start,
      .stop = mod_a_stop, .deinit = mod_a_deinit },
};
const uint32_t _bm_module_count = 2;

void setUp(void) {
    BM_LOGI("test_mod", "setUp: reset lifecycle counters");
    g_init_count = 0;
    g_start_count = 0;
    g_stop_count = 0;
    g_deinit_count = 0;
    g_fail_mod_b_init = 0;
    g_fail_mod_b_start = 0;
}
void tearDown(void) {
    bm_module_deinit_all();
}

void test_module_init_failure_rollback(void) {
    g_fail_mod_b_init = 1;
    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_module_init_all());
    TEST_ASSERT_EQUAL(2, g_init_count);
    TEST_ASSERT_EQUAL(1, g_deinit_count);
    TEST_ASSERT_EQUAL(BM_OK, bm_module_deinit_all());
    TEST_ASSERT_EQUAL(1, g_deinit_count);
}

void test_module_start_failure_rollback(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_module_init_all());
    g_fail_mod_b_start = 1;
    TEST_ASSERT_EQUAL(BM_ERR_INVALID, bm_module_start_all());
    TEST_ASSERT_EQUAL(2, g_start_count);
    TEST_ASSERT_EQUAL(1, g_stop_count);
}

void test_module_lifecycle_order(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_module_init_all());
    TEST_ASSERT_EQUAL(2, g_init_count);
    TEST_ASSERT_EQUAL(BM_OK, bm_module_start_all());
    TEST_ASSERT_EQUAL(2, g_start_count);

    TEST_ASSERT_EQUAL(BM_OK, bm_module_stop_all());
    TEST_ASSERT_EQUAL(1, g_stop_count); /* only mod_a has stop */

    TEST_ASSERT_EQUAL(BM_OK, bm_module_deinit_all());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_module_init_failure_rollback);
    RUN_TEST(test_module_start_failure_rollback);
    RUN_TEST(test_module_lifecycle_order);
    return UNITY_END();
}
