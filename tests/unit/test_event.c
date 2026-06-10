#include "unity.h"
#include "bm_core.h"

static int g_count = 0;
static bm_event_t g_last_event;
static uint16_t g_last_data = 0;
static uint8_t g_seen_data[4];
static bm_event_type_t g_seen_types[4];

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
    if (ev->data && ev->data_len == sizeof(g_last_data)) {
        memcpy(&g_last_data, ev->data, sizeof(g_last_data));
    }
    g_last_event.data = NULL;
}

void setUp(void) {
    bm_event_reset();
    g_count = 0;
    g_last_data = 0;
    memset(&g_last_event, 0, sizeof(g_last_event));
    memset(g_seen_data, 0, sizeof(g_seen_data));
    memset(g_seen_types, 0, sizeof(g_seen_types));
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
    bm_event_process(1); /* only process one */
    TEST_ASSERT_EQUAL(EVENT_HIGH, g_last_event.type);

    bm_event_process(1); /* process second */
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
    TEST_ASSERT_EQUAL(0, g_count); /* unsubscribed, should not fire */
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

void test_event_rejects_invalid_payload_and_priority(void) {
    TEST_ASSERT_EQUAL(BM_ERR_INVALID,
        bm_event_publish_copy(EVENT_TEST, 0, NULL, 1));
    TEST_ASSERT_EQUAL(BM_ERR_INVALID,
        bm_event_publish_copy(EVENT_TEST, 4, NULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_event_publish_copy_and_process);
    RUN_TEST(test_event_priority_order);
    RUN_TEST(test_event_unsubscribe_by_id);
    RUN_TEST(test_event_priority_reorder_preserves_inline_data);
    RUN_TEST(test_event_rejects_invalid_payload_and_priority);
    return UNITY_END();
}
