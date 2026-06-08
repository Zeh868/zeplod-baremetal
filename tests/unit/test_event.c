#include "unity.h"
#include "bm_core.h"

static int g_count = 0;
static bm_event_t g_last_event;

#define EVENT_TEST 1
#define EVENT_HIGH 2

static void test_cb(const bm_event_t *ev, void *user_data) {
    int *count = (int *)user_data;
    (*count)++;
    g_last_event = *ev;
}

void setUp(void) {
    g_count = 0;
    memset(&g_last_event, 0, sizeof(g_last_event));
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
    TEST_ASSERT_EQUAL(42, *(const uint16_t *)g_last_event.data);

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_event_publish_copy_and_process);
    RUN_TEST(test_event_priority_order);
    RUN_TEST(test_event_unsubscribe_by_id);
    return UNITY_END();
}
