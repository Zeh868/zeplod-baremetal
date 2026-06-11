/**
 * @file test_log.c
 * @brief 日志格式化边界单元测试
 */
#include "unity.h"
#include "bm_log.h"

#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
static char   g_log_buf[256];
static size_t g_log_len;

__attribute__((weak))
void bm_log_output(const char *buf, size_t len) {
    g_log_len = len;
    if (len >= sizeof(g_log_buf)) {
        len = sizeof(g_log_buf) - 1u;
    }
    memcpy(g_log_buf, buf, len);
    g_log_buf[len] = '\0';
}
#endif

void setUp(void) {
#if defined(__GNUC__) || defined(__clang__)
    g_log_len = 0u;
    memset(g_log_buf, 0, sizeof(g_log_buf));
#endif
}
void tearDown(void) {}

void test_log_null_fmt_no_output(void) {
    bm_log(BM_LOG_INFO, "t", NULL);
#if defined(__GNUC__) || defined(__clang__)
    TEST_ASSERT_EQUAL(0u, g_log_len);
#endif
}

void test_log_truncation_drops_output(void) {
#if defined(__GNUC__) || defined(__clang__)
    char long_fmt[200];

    memset(long_fmt, 'A', sizeof(long_fmt) - 1u);
    long_fmt[sizeof(long_fmt) - 1u] = '\0';

    bm_log(BM_LOG_INFO, "t", long_fmt);
    TEST_ASSERT_EQUAL(0u, g_log_len);
#else
    TEST_PASS();
#endif
}

void test_log_basic_message(void) {
#if defined(__GNUC__) || defined(__clang__)
    bm_log(BM_LOG_INFO, "tag", "hello");
    TEST_ASSERT_GREATER_THAN(0, (int)g_log_len);
    TEST_ASSERT_NOT_NULL(strstr(g_log_buf, "hello"));
#else
    bm_log(BM_LOG_INFO, "tag", "hello");
    TEST_PASS();
#endif
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_log_null_fmt_no_output);
    RUN_TEST(test_log_truncation_drops_output);
    RUN_TEST(test_log_basic_message);
    return UNITY_END();
}
