/**
 * @file test_mempool.c
 * @brief 固定大小内存池分配、释放与耗尽单元测试
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "unity.h"
#include "bm_core.h"
#include "bm_log.h"

#include <string.h>

typedef struct { uint32_t a; uint8_t b; } test_obj_t;

void setUp(void) {
    BM_LOGI("test_pool", "setUp");
}
void tearDown(void) {}

void test_mempool_alloc_free(void) {
    BM_MEMPOOL_DEFINE(pool, test_obj_t, 4);
    memset(pool.bitmap, 0, pool.bitmap_words * sizeof(uint32_t));

    test_obj_t *a = (test_obj_t *)bm_mempool_alloc(&pool);
    TEST_ASSERT_NOT_NULL(a);
    a->a = 123;

    bm_mempool_free(&pool, a);
    test_obj_t *b = (test_obj_t *)bm_mempool_alloc(&pool);
    TEST_ASSERT_EQUAL_PTR(a, b);
}

void test_mempool_exhausted(void) {
    BM_MEMPOOL_DEFINE(pool, test_obj_t, 2);
    memset(pool.bitmap, 0, pool.bitmap_words * sizeof(uint32_t));

    TEST_ASSERT_NOT_NULL(bm_mempool_alloc(&pool));
    TEST_ASSERT_NOT_NULL(bm_mempool_alloc(&pool));
    BM_LOGE("test_pool", "expect NULL when pool exhausted");
    TEST_ASSERT_NULL(bm_mempool_alloc(&pool));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mempool_alloc_free);
    RUN_TEST(test_mempool_exhausted);
    return UNITY_END();
}
