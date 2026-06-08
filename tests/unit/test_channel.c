#include "unity.h"
#include "bm_channel.h"

typedef struct {
    uint32_t a;
    uint16_t b;
} test_msg_t;

BM_CHANNEL_DEFINE(my_chan, test_msg_t, 4);

void setUp(void) {
    bm_channel_reset(&my_chan);
}
void tearDown(void) {}

void test_channel_send_recv(void) {
    test_msg_t msg = { .a = 123, .b = 456 };
    TEST_ASSERT_EQUAL(BM_OK, bm_channel_send(&my_chan, &msg));
    TEST_ASSERT_EQUAL(1, bm_channel_count(&my_chan));
    TEST_ASSERT_FALSE(bm_channel_is_empty(&my_chan));

    test_msg_t out = {0};
    TEST_ASSERT_EQUAL(BM_OK, bm_channel_recv(&my_chan, &out));
    TEST_ASSERT_EQUAL(123, out.a);
    TEST_ASSERT_EQUAL(456, out.b);
    TEST_ASSERT_TRUE(bm_channel_is_empty(&my_chan));
}

void test_channel_overflow(void) {
    /* capacity 4, but one slot reserved to distinguish full/empty */
    test_msg_t msg = { .a = 1, .b = 2 };
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL(BM_OK, bm_channel_send(&my_chan, &msg));
    }
    TEST_ASSERT_TRUE(bm_channel_is_full(&my_chan));
    TEST_ASSERT_EQUAL(BM_ERR_OVERFLOW, bm_channel_send(&my_chan, &msg));
}

void test_channel_underflow(void) {
    test_msg_t out = {0};
    TEST_ASSERT_TRUE(bm_channel_is_empty(&my_chan));
    TEST_ASSERT_EQUAL(BM_ERR_WOULD_BLOCK, bm_channel_recv(&my_chan, &out));
}

void test_channel_wraparound(void) {
    test_msg_t msg = { .a = 42, .b = 0 };
    for (int i = 0; i < 3; i++) {
        msg.a = i;
        TEST_ASSERT_EQUAL(BM_OK, bm_channel_send(&my_chan, &msg));
    }

    /* Receive 2, send 2 to force wrap */
    test_msg_t out;
    bm_channel_recv(&my_chan, &out);
    bm_channel_recv(&my_chan, &out);

    msg.a = 100;
    TEST_ASSERT_EQUAL(BM_OK, bm_channel_send(&my_chan, &msg));
    msg.a = 101;
    TEST_ASSERT_EQUAL(BM_OK, bm_channel_send(&my_chan, &msg));

    /* Drain and verify order */
    bm_channel_recv(&my_chan, &out);
    TEST_ASSERT_EQUAL(2, out.a);
    bm_channel_recv(&my_chan, &out);
    TEST_ASSERT_EQUAL(100, out.a);
    bm_channel_recv(&my_chan, &out);
    TEST_ASSERT_EQUAL(101, out.a);
    TEST_ASSERT_TRUE(bm_channel_is_empty(&my_chan));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_channel_send_recv);
    RUN_TEST(test_channel_overflow);
    RUN_TEST(test_channel_underflow);
    RUN_TEST(test_channel_wraparound);
    return UNITY_END();
}
