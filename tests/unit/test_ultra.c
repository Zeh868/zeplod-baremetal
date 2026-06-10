/**
 * @file test_ultra.c
 * @brief Ultra 轻量事件发布、处理与队列溢出单元测试
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "unity.h"
#include "bm_ultra.h"
#include "bm_log.h"

static int g_count = 0;

#define EVENT_TEST 1

static void test_cb(const void *data, uint8_t len) {
    g_count++;
    if (data && len == sizeof(uint16_t)) {
        uint16_t val = *(const uint16_t *)data;
        TEST_ASSERT_EQUAL(42, val);
    }
}

BM_ULTRA_CALLBACK_TABLE_DEFINE(
    BM_ULTRA_CB(EVENT_TEST, test_cb)
);

void setUp(void) {
    BM_LOGI("test_ultra", "setUp: reset ultra subsystem");
    g_count = 0;
    bm_ultra_init();
}
void tearDown(void) {}

void test_ultra_publish_and_process(void) {
    uint16_t data = 42;
    TEST_ASSERT_EQUAL(0, bm_ultra_publish(EVENT_TEST, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(1, bm_ultra_event_count());
    TEST_ASSERT_EQUAL(1, bm_ultra_process());
    TEST_ASSERT_EQUAL(1, g_count);
    TEST_ASSERT_EQUAL(0, bm_ultra_event_count());
}

void test_ultra_queue_overflow(void) {
    for (int i = 0; i < BM_CONFIG_ULTRA_QUEUE_DEPTH + 2; i++) {
        uint8_t dummy = (uint8_t)i;
        bm_ultra_publish(EVENT_TEST, &dummy, sizeof(dummy));
    }
    uint8_t processed = 0;
    while (bm_ultra_process()) {
        processed++;
    }
    /* Queue depth 8, but we use one slot to distinguish full/empty,
       so max storable = 7 */
    TEST_ASSERT_EQUAL(BM_CONFIG_ULTRA_QUEUE_DEPTH - 1, processed);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ultra_publish_and_process);
    RUN_TEST(test_ultra_queue_overflow);
    return UNITY_END();
}
