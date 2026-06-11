/**
 * @file test_resource_claim.c
 * @brief 资源声明冲突检测（独占/共享/协调）单元测试
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "unity.h"
#include "bm_resource.h"
#include "bm_log.h"

/* 两实例争抢同一 PWM 通道（独占） */
static const bm_resource_claim_t inst_a_claims[] = {
    { BM_RESOURCE_PWM, 1u, BM_RESOURCE_EXCLUSIVE, 0u, "pwm" },
};

static const bm_resource_claim_t inst_b_claims[] = {
    { BM_RESOURCE_PWM, 1u, BM_RESOURCE_EXCLUSIVE, 0u, "pwm" },
};

static const bm_resource_claim_t owner_claims[] = {
    { BM_RESOURCE_ADC_GROUP, 2u, BM_RESOURCE_OWNER, 10u, "adc owner" },
};

static const bm_resource_claim_t reader_claims[] = {
    { BM_RESOURCE_ADC_GROUP, 2u, BM_RESOURCE_SHARED_READ, 10u, "adc reader" },
};

static const bm_resource_claim_t reader_no_owner[] = {
    { BM_RESOURCE_ADC_GROUP, 2u, BM_RESOURCE_SHARED_READ, 10u, "reader only" },
};

void setUp(void) {
    BM_LOGI("test_res", "setUp");
}
void tearDown(void) {}

void test_resource_exclusive_conflict(void) {
    const bm_resource_claim_t *claims[] = { inst_a_claims, inst_b_claims };
    uint32_t counts[] = { 1u, 1u };

    BM_LOGE("test_res", "expect BUSY on exclusive PWM conflict");
    TEST_ASSERT_EQUAL(BM_ERR_BUSY,
                      bm_resource_check_conflicts(claims, counts, 2u));
}

void test_resource_owner_reader_allowed(void) {
    const bm_resource_claim_t *claims[] = { owner_claims, reader_claims };
    uint32_t counts[] = { 1u, 1u };

    TEST_ASSERT_EQUAL(BM_OK,
                      bm_resource_check_conflicts(claims, counts, 2u));
}

void test_resource_reader_without_owner_fails(void) {
    const bm_resource_claim_t *claims[] = { reader_no_owner };
    uint32_t counts[] = { 1u };

    TEST_ASSERT_EQUAL(BM_ERR_INVALID,
                      bm_resource_check_conflicts(claims, counts, 1u));
}

void test_resource_coordinated_shared(void) {
    static const bm_resource_claim_t coord_a[] = {
        { BM_RESOURCE_TIMER, 3u, BM_RESOURCE_SHARED_COORDINATED, 5u, "t" },
    };
    static const bm_resource_claim_t coord_b[] = {
        { BM_RESOURCE_TIMER, 3u, BM_RESOURCE_SHARED_COORDINATED, 5u, "t" },
    };
    const bm_resource_claim_t *claims[] = { coord_a, coord_b };
    uint32_t counts[] = { 1u, 1u };

    TEST_ASSERT_EQUAL(BM_OK,
                      bm_resource_check_conflicts(claims, counts, 2u));
}

void test_resource_duplicate_claim_same_instance_fails(void) {
    static const bm_resource_claim_t dup_claims[] = {
        { BM_RESOURCE_PWM, 1u, BM_RESOURCE_EXCLUSIVE, 0u, "pwm_a" },
        { BM_RESOURCE_PWM, 1u, BM_RESOURCE_EXCLUSIVE, 0u, "pwm_b" },
    };
    const bm_resource_claim_t *claims[] = { dup_claims };
    uint32_t counts[] = { 2u };

    TEST_ASSERT_EQUAL(BM_ERR_INVALID,
                      bm_resource_check_conflicts(claims, counts, 1u));
}

void test_resource_shared_read_vs_coordinated_conflict(void) {
    static const bm_resource_claim_t read_claim[] = {
        { BM_RESOURCE_TIMER, 9u, BM_RESOURCE_SHARED_READ, 3u, "read" },
    };
    static const bm_resource_claim_t owner_claim[] = {
        { BM_RESOURCE_TIMER, 9u, BM_RESOURCE_OWNER, 3u, "owner" },
    };
    static const bm_resource_claim_t coord_claim[] = {
        { BM_RESOURCE_TIMER, 9u, BM_RESOURCE_SHARED_COORDINATED, 4u, "coord" },
    };
    const bm_resource_claim_t *claims[] = {
        owner_claim, read_claim, coord_claim
    };
    uint32_t counts[] = { 1u, 1u, 1u };

    TEST_ASSERT_EQUAL(BM_ERR_BUSY,
                      bm_resource_check_conflicts(claims, counts, 3u));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_resource_exclusive_conflict);
    RUN_TEST(test_resource_owner_reader_allowed);
    RUN_TEST(test_resource_reader_without_owner_fails);
    RUN_TEST(test_resource_coordinated_shared);
    RUN_TEST(test_resource_duplicate_claim_same_instance_fails);
    RUN_TEST(test_resource_shared_read_vs_coordinated_conflict);
    return UNITY_END();
}
