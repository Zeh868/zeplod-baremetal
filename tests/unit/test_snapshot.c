#include "unity.h"
#include "bm_snapshot.h"

BM_SNAPSHOT_DEFINE(test_box, uint32_t);

static test_box_snapshot_t g_box = BM_SNAPSHOT_INITIALIZER;

void setUp(void) {
    g_box.published = 0u;
    g_box.reading = BM_SNAPSHOT_NONE;
    g_box.buffer[0] = 0u;
    g_box.buffer[1] = 0u;
    g_box.buffer[2] = 0u;
}

void tearDown(void) {}

void test_snapshot_publish_read_roundtrip(void) {
    uint32_t in = 12345u;
    uint32_t out = 0u;

    BM_SNAPSHOT_PUBLISH(g_box, &in);
    BM_SNAPSHOT_READ(g_box, &out);
    TEST_ASSERT_EQUAL_UINT32(in, out);
}

void test_snapshot_reader_never_sees_partial_publish(void) {
    uint32_t values[3] = { 1u, 2u, 3u };
    uint32_t out;
    uint32_t i;

    for (i = 0u; i < 3u; ++i) {
        BM_SNAPSHOT_PUBLISH(g_box, &values[i]);
    }
    BM_SNAPSHOT_READ(g_box, &out);
    TEST_ASSERT_EQUAL_UINT32(3u, out);
}

void test_snapshot_choose_buffer_skips_active(void) {
    TEST_ASSERT_EQUAL_UINT8(1u, bm_snapshot_choose_buffer(0u, BM_SNAPSHOT_NONE));
    TEST_ASSERT_EQUAL_UINT8(2u, bm_snapshot_choose_buffer(0u, 1u));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_snapshot_publish_read_roundtrip);
    RUN_TEST(test_snapshot_reader_never_sees_partial_publish);
    RUN_TEST(test_snapshot_choose_buffer_skips_active);
    return UNITY_END();
}
