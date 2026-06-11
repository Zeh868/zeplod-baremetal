/**
 * @file test_ticker.c
 * @brief 软定时 Ticker 周期发布、队列溢出与 tick 回绕单元测试
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "unity.h"
#include "bm_ticker.h"
#include "bm_log.h"
#include "bm_hal_timer_native.h"

#define TICKER_EVT 3u

static int g_event_count;

static void ticker_cb(const bm_event_t *ev, void *user_data) {
    (void)user_data;
    if (ev->type == TICKER_EVT) {
        g_event_count++;
    }
}

void setUp(void) {
    BM_LOGI("test_ticker", "setUp: reset event and ticker");
    g_event_count = 0;
    bm_event_reset();
    bm_hal_timer_native_reset_ticks();
    bm_ticker_reset();
    bm_hal_timer_init(1000u);
}

void tearDown(void) {
    bm_ticker_reset();
}

void test_ticker_publishes_on_period(void) {
    static const bm_ticker_slot_t slots[] = {
        { 10u, TICKER_EVT, 1u, "10ms" },
    };
    bm_event_subscriber_id_t id;

    bm_event_register_type(TICKER_EVT, "TICK");
    bm_event_subscribe(TICKER_EVT, ticker_cb, NULL, &id);
    TEST_ASSERT_EQUAL(BM_OK, bm_ticker_init(slots, 1u));

    bm_hal_timer_native_advance_ticks(10u);
    TEST_ASSERT_EQUAL(1, bm_ticker_poll());
    TEST_ASSERT_EQUAL(1, bm_event_process(4));
    TEST_ASSERT_EQUAL(1, g_event_count);

    bm_hal_timer_native_advance_ticks(10u);
    TEST_ASSERT_EQUAL(1, bm_ticker_poll());
    TEST_ASSERT_EQUAL(1, bm_event_process(4));
    TEST_ASSERT_EQUAL(2, g_event_count);

    bm_event_unsubscribe(TICKER_EVT, id);
}

void test_ticker_counts_dropped_when_queue_full(void) {
    static const bm_ticker_slot_t slots[] = {
        { 10u, TICKER_EVT, 1u, "10ms" },
    };
    uint32_t i;

    bm_event_register_type(TICKER_EVT, "TICK");
    i = 0u;
    while (bm_event_publish_copy(TICKER_EVT, 1u, NULL, 0u) == BM_OK) {
        i++;
    }
    TEST_ASSERT_GREATER_THAN(0u, i);
    TEST_ASSERT_EQUAL(BM_ERR_OVERFLOW,
                      bm_event_publish_copy(TICKER_EVT, 1u, NULL, 0u));

    TEST_ASSERT_EQUAL(BM_OK, bm_ticker_init(slots, 1u));
    bm_hal_timer_native_advance_ticks(20u);
    (void)bm_ticker_poll();
    BM_LOGE("test_ticker", "expect dropped count when event queue full");
    TEST_ASSERT_GREATER_THAN(0u, bm_ticker_get_dropped(0u));
}

void test_ticker_wraparound(void) {
    static const bm_ticker_slot_t slots[] = {
        { 10u, TICKER_EVT, 1u, "10ms" },
    };
    bm_event_subscriber_id_t id;

    bm_event_register_type(TICKER_EVT, "TICK");
    bm_event_subscribe(TICKER_EVT, ticker_cb, NULL, &id);

    bm_hal_timer_native_advance_ticks(0xFFFFFFF0u);
    TEST_ASSERT_EQUAL(BM_OK, bm_ticker_init(slots, 1u));
    bm_hal_timer_native_advance_ticks(10u);
    TEST_ASSERT_GREATER_OR_EQUAL(1, bm_ticker_poll());
    TEST_ASSERT_GREATER_OR_EQUAL(1, bm_event_process(4));
    TEST_ASSERT_GREATER_OR_EQUAL(1, g_event_count);

    bm_event_unsubscribe(TICKER_EVT, id);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ticker_publishes_on_period);
    RUN_TEST(test_ticker_counts_dropped_when_queue_full);
    RUN_TEST(test_ticker_wraparound);
    return UNITY_END();
}
