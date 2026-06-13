/**
 * @file test_stream.c
 * @brief bm_stream 零拷贝块流单元测试
 *
 * 覆盖生产者/消费者往返、ready 回调与 drop-newest 过载策略。
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-12
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-12       1.0            zeh            正式发布
 */
#include "unity.h"
#include "bm_stream.h"
#include "bm_config.h"

#include <string.h>

typedef struct {
    uint32_t samples[8];
} test_payload_t;

static test_payload_t s_payloads[2];
static bm_block_t s_blocks[2];
static bm_stream_t s_stream;

static uint32_t g_ready_count;
static uint32_t g_last_sequence;

static void on_ready(bm_stream_t *stream, bm_block_t *block, void *context) {
    (void)stream;
    (void)context;
    g_ready_count++;
    g_last_sequence = block->sequence;
}

void setUp(void) {
    g_ready_count = 0u;
    g_last_sequence = 0u;
    memset(&s_stream, 0, sizeof(s_stream));
    s_stream.blocks = s_blocks;
    s_stream.block_count = 2u;
    s_stream.policy = BM_STREAM_POLICY_DROP_NEWEST;
    bm_stream_init(&s_stream, s_payloads, 2u, sizeof(test_payload_t));
    bm_stream_reset(&s_stream);
}

void tearDown(void) {
    bm_stream_set_ready_handler(&s_stream, NULL, NULL);
}

void test_stream_producer_consumer_roundtrip(void) {
    bm_block_t *prod;
    bm_block_t *cons;
    test_payload_t *payload;

    TEST_ASSERT_EQUAL(BM_OK, bm_stream_producer_acquire(&s_stream, &prod));
    payload = (test_payload_t *)prod->data;
    payload->samples[0] = 42u;
    TEST_ASSERT_EQUAL(BM_OK,
                      bm_stream_producer_commit(&s_stream, prod,
                                                sizeof(test_payload_t), NULL));
    TEST_ASSERT_EQUAL(BM_OK, bm_stream_consumer_acquire(&s_stream, &cons));
    TEST_ASSERT_EQUAL_PTR(prod, cons);
    TEST_ASSERT_EQUAL(42u, ((test_payload_t *)cons->data)->samples[0]);
    TEST_ASSERT_EQUAL(BM_OK, bm_stream_consumer_release(&s_stream, cons));
}

void test_stream_ready_handler_fires(void) {
    bm_block_t *prod;

    bm_stream_set_ready_handler(&s_stream, on_ready, NULL);
    TEST_ASSERT_EQUAL(BM_OK, bm_stream_producer_acquire(&s_stream, &prod));
    TEST_ASSERT_EQUAL(BM_OK,
                      bm_stream_producer_commit(&s_stream, prod, 16u, NULL));
    TEST_ASSERT_EQUAL(1u, g_ready_count);
    TEST_ASSERT_EQUAL(1u, g_last_sequence);
}

void test_stream_drop_newest_on_overrun(void) {
    bm_block_t *b0;
    bm_block_t *b1;
    bm_block_t *extra;

    TEST_ASSERT_EQUAL(BM_OK, bm_stream_producer_acquire(&s_stream, &b0));
    TEST_ASSERT_EQUAL(BM_OK,
                      bm_stream_producer_commit(&s_stream, b0, 8u, NULL));
    TEST_ASSERT_EQUAL(BM_OK, bm_stream_producer_acquire(&s_stream, &b1));
    TEST_ASSERT_EQUAL(BM_OK,
                      bm_stream_producer_commit(&s_stream, b1, 8u, NULL));
    TEST_ASSERT_EQUAL(BM_ERR_OVERFLOW,
                      bm_stream_producer_acquire(&s_stream, &extra));
    TEST_ASSERT_EQUAL(1u, bm_stream_stats(&s_stream)->overrun);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stream_producer_consumer_roundtrip);
    RUN_TEST(test_stream_ready_handler_fires);
    RUN_TEST(test_stream_drop_newest_on_overrun);
    return UNITY_END();
}
