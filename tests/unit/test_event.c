/**
 * @file test_event.c
 * @brief 事件总线 publish/subscribe、优先级与内联数据单元测试
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

static int g_count = 0;
static bm_event_t g_last_event;
static uint16_t g_last_data = 0;
static uint8_t g_seen_data[4];
static bm_event_type_t g_seen_types[4];
static uintptr_t g_last_data_address;
static int g_reentrant_publish_rc;
static int g_reentrant_subscribe_rc;
static int g_reentrant_process_rc;

#define EVENT_TEST 1
#define EVENT_HIGH 2

static void test_cb(const bm_event_t *ev, void *user_data) {
    int *count = (int *)user_data;
    if (*count < 4) {
        g_seen_types[*count] = ev->type;
        g_seen_data[*count] = ev->data ? *(const uint8_t *)ev->data : 0;
    }
    (*count)++;
    g_last_event = *ev;
    g_last_data_address = (uintptr_t)ev->data;
    if (ev->data && ev->data_len == sizeof(g_last_data)) {
        memcpy(&g_last_data, ev->data, sizeof(g_last_data));
    }
    g_last_event.data = NULL;
}

static void reentrant_cb(const bm_event_t *ev, void *user_data) {
    (void)ev;
    (void)user_data;
    g_reentrant_publish_rc =
        bm_event_publish_copy(EVENT_TEST, 0u, NULL, 0u);
    g_reentrant_subscribe_rc =
        bm_event_subscribe(EVENT_TEST, test_cb, &g_count, NULL);
    bm_event_reset();
    g_reentrant_process_rc = bm_event_process(1u);
}

void setUp(void) {
    BM_LOGI("test_evt", "setUp: reset event subsystem");
    bm_event_reset();
    g_count = 0;
    g_last_data = 0;
    memset(&g_last_event, 0, sizeof(g_last_event));
    memset(g_seen_data, 0, sizeof(g_seen_data));
    memset(g_seen_types, 0, sizeof(g_seen_types));
    g_last_data_address = 0u;
    g_reentrant_publish_rc = BM_OK;
    g_reentrant_subscribe_rc = BM_OK;
    g_reentrant_process_rc = BM_OK;
}
void tearDown(void) {}

void test_event_publish_copy_and_process(void) {
    bm_event_subscriber_id_t id;
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_subscribe(EVENT_TEST, test_cb, &g_count, &id));

    uint16_t data = 42;
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 1, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(1, bm_event_process(8));
    TEST_ASSERT_EQUAL(1, g_count);
    TEST_ASSERT_EQUAL(EVENT_TEST, g_last_event.type);
    TEST_ASSERT_EQUAL(sizeof(data), g_last_event.data_len);
    TEST_ASSERT_EQUAL(42, g_last_data);
    TEST_ASSERT_EQUAL(0u, g_last_data_address % 8u);

    bm_event_unsubscribe(EVENT_TEST, id);
}

void test_event_priority_order(void) {
    bm_event_subscriber_id_t id1, id2;
    bm_event_register_type(EVENT_TEST, "TEST");
    bm_event_register_type(EVENT_HIGH, "HIGH");
    bm_event_subscribe(EVENT_TEST, test_cb, &g_count, &id1);
    bm_event_subscribe(EVENT_HIGH, test_cb, &g_count, &id2);

    uint8_t low = 1, high = 2;
    bm_event_publish_copy(EVENT_TEST, 2, &low, sizeof(low));
    bm_event_publish_copy(EVENT_HIGH, 0, &high, sizeof(high));

    g_count = 0;
    bm_event_process(1); /* 仅处理一条 */
    TEST_ASSERT_EQUAL(EVENT_HIGH, g_last_event.type);

    bm_event_process(1); /* 处理第二条 */
    TEST_ASSERT_EQUAL(EVENT_TEST, g_last_event.type);

    bm_event_unsubscribe(EVENT_TEST, id1);
    bm_event_unsubscribe(EVENT_HIGH, id2);
}

void test_event_unsubscribe_by_id(void) {
    bm_event_subscriber_id_t id;
    bm_event_register_type(EVENT_TEST, "TEST");
    bm_event_subscribe(EVENT_TEST, test_cb, &g_count, &id);
    TEST_ASSERT_EQUAL(BM_OK, bm_event_unsubscribe(EVENT_TEST, id));

    uint8_t data = 1;
    bm_event_publish_copy(EVENT_TEST, 0, &data, sizeof(data));
    bm_event_process(8);
    TEST_ASSERT_EQUAL(0, g_count); /* 已退订，不应触发 */
}

void test_event_priority_reorder_preserves_inline_data(void) {
    bm_event_subscriber_id_t id1, id2;
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_HIGH, "HIGH"));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_subscribe(EVENT_TEST, test_cb, &g_count, &id1));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_subscribe(EVENT_HIGH, test_cb, &g_count, &id2));

    uint8_t low = 11;
    uint8_t high = 22;
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 2, &low, sizeof(low)));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_HIGH, 0, &high, sizeof(high)));

    TEST_ASSERT_EQUAL(2, bm_event_process(2));
    TEST_ASSERT_EQUAL(EVENT_HIGH, g_seen_types[0]);
    TEST_ASSERT_EQUAL(22, g_seen_data[0]);
    TEST_ASSERT_EQUAL(EVENT_TEST, g_seen_types[1]);
    TEST_ASSERT_EQUAL(11, g_seen_data[1]);
}

void test_event_queue_overflow_counts_dropped(void) {
    uint32_t i = 0u;
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    while (bm_event_publish_copy(EVENT_TEST, 0, NULL, 0u) == BM_OK) {
        i++;
    }
    TEST_ASSERT_GREATER_THAN(0u, i);
    TEST_ASSERT_GREATER_THAN(0u, bm_event_get_dropped_count());
}

void test_event_requires_registered_type(void) {
    TEST_ASSERT_EQUAL(BM_ERR_NOT_INIT,
        bm_event_subscribe(EVENT_TEST, test_cb, &g_count, NULL));
    TEST_ASSERT_EQUAL(BM_ERR_NOT_INIT,
        bm_event_publish_copy(EVENT_TEST, 0u, NULL, 0u));
}

void test_event_rejects_callback_reentrancy(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_subscribe(EVENT_TEST, reentrant_cb, NULL, NULL));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 0u, NULL, 0u));

    TEST_ASSERT_EQUAL(1, bm_event_process(1u));
    TEST_ASSERT_EQUAL(BM_ERR_BUSY, g_reentrant_publish_rc);
    TEST_ASSERT_EQUAL(BM_ERR_BUSY, g_reentrant_subscribe_rc);
    TEST_ASSERT_EQUAL(BM_ERR_BUSY, g_reentrant_process_rc);
    TEST_ASSERT_EQUAL(4u, bm_event_get_reentrancy_rejected_count());
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 0u, NULL, 0u));
}

void test_event_priority_fairness_bound(void) {
    uint32_t i;

    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "LOW"));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_HIGH, "HIGH"));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_subscribe(EVENT_TEST, test_cb, &g_count, NULL));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_subscribe(EVENT_HIGH, test_cb, &g_count, NULL));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 3u, NULL, 0u));

    for (i = 0u; i < BM_CONFIG_EVENT_PRIORITY_BURST_MAX; i++) {
        TEST_ASSERT_EQUAL(BM_OK,
            bm_event_publish_copy(EVENT_HIGH, 0u, NULL, 0u));
        TEST_ASSERT_EQUAL(1, bm_event_process(1u));
        TEST_ASSERT_EQUAL(EVENT_HIGH, g_last_event.type);
    }

    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_HIGH, 0u, NULL, 0u));
    TEST_ASSERT_EQUAL(1, bm_event_process(1u));
    TEST_ASSERT_EQUAL(EVENT_TEST, g_last_event.type);
}

void test_event_fairness_ignores_uncontended_history(void) {
    uint32_t i;

    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "LOW"));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_HIGH, "HIGH"));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_subscribe(EVENT_TEST, test_cb, &g_count, NULL));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_subscribe(EVENT_HIGH, test_cb, &g_count, NULL));

    for (i = 0u; i < BM_CONFIG_EVENT_PRIORITY_BURST_MAX; i++) {
        TEST_ASSERT_EQUAL(BM_OK,
            bm_event_publish_copy(EVENT_TEST, 3u, NULL, 0u));
        TEST_ASSERT_EQUAL(1, bm_event_process(1u));
    }

    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_HIGH, 0u, NULL, 0u));
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 3u, NULL, 0u));
    TEST_ASSERT_EQUAL(1, bm_event_process(1u));
    TEST_ASSERT_EQUAL(EVENT_HIGH, g_last_event.type);
}

void test_event_register_type_rejects_duplicate(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    TEST_ASSERT_EQUAL(BM_ERR_ALREADY, bm_event_register_type(EVENT_TEST, "TEST2"));
}

void test_event_rejects_invalid_payload_and_priority(void) {
    BM_LOGE("test_evt", "expect INVALID for bad payload/priority");
    TEST_ASSERT_EQUAL(BM_ERR_INVALID,
        bm_event_publish_copy(EVENT_TEST, 0, NULL, 1));
    TEST_ASSERT_EQUAL(BM_ERR_INVALID,
        bm_event_publish_copy(EVENT_TEST, 4, NULL, 0));
}

void test_event_dispatch_skipped_invalid_type(void) {
    bm_event_t bad = {
        .type = (bm_event_type_t)BM_CONFIG_MAX_EVENT_TYPES,
        .priority = 0,
        .data = NULL,
        .data_len = 0u,
        .source_id = 0,
    };

    TEST_ASSERT_EQUAL(0u, bm_event_get_dispatch_skipped_count());
    TEST_ASSERT_EQUAL(BM_OK, bm_event_test_inject(&bad, 0));
    TEST_ASSERT_EQUAL(1, bm_event_process(1));
    TEST_ASSERT_EQUAL(1u, bm_event_get_dispatch_skipped_count());
}

void test_event_subscribe_null_id(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_subscribe(EVENT_TEST, test_cb, &g_count, NULL));

    uint8_t data = 7;
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 0, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(1, bm_event_process(8));
    TEST_ASSERT_EQUAL(1, g_count);
    TEST_ASSERT_EQUAL(7, g_seen_data[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_event_publish_copy_and_process);
    RUN_TEST(test_event_priority_order);
    RUN_TEST(test_event_unsubscribe_by_id);
    RUN_TEST(test_event_priority_reorder_preserves_inline_data);
    RUN_TEST(test_event_rejects_invalid_payload_and_priority);
    RUN_TEST(test_event_queue_overflow_counts_dropped);
    RUN_TEST(test_event_requires_registered_type);
    RUN_TEST(test_event_rejects_callback_reentrancy);
    RUN_TEST(test_event_priority_fairness_bound);
    RUN_TEST(test_event_fairness_ignores_uncontended_history);
    RUN_TEST(test_event_dispatch_skipped_invalid_type);
    RUN_TEST(test_event_register_type_rejects_duplicate);
    RUN_TEST(test_event_subscribe_null_id);
    return UNITY_END();
}
