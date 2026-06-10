#include "unity.h"
#include "bm_resource.h"

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

void setUp(void) {}
void tearDown(void) {}

void test_resource_exclusive_conflict(void) {
    const bm_resource_claim_t *claims[] = { inst_a_claims, inst_b_claims };
    uint32_t counts[] = { 1u, 1u };

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_resource_exclusive_conflict);
    RUN_TEST(test_resource_owner_reader_allowed);
    RUN_TEST(test_resource_reader_without_owner_fails);
    RUN_TEST(test_resource_coordinated_shared);
    return UNITY_END();
}
