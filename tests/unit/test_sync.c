#include "unity.h"
#include "bm_sync.h"

struct bm_hal_timer {
    int placeholder;
};

static const struct bm_hal_timer dummy_timer_storage;
static const bm_ctrl_inst_t dummy_member;
static const bm_ctrl_inst_t *const members[] = { &dummy_member };
static const uint32_t phases[] = { 0u };

static bm_sync_domain_t domain;

static void init_domain(void) {
    domain.name = "test";
    domain.master_timer = &dummy_timer_storage;
    domain.members = members;
    domain.phase_ticks = phases;
    domain.member_count = 1u;
}

void setUp(void) {
    init_domain();
    bm_sync_safe_stop(&domain);
}

void tearDown(void) {
    bm_sync_safe_stop(&domain);
}

void test_sync_configure_arm_trigger(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_arm(&domain));
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_trigger(&domain));
}

void test_sync_trigger_before_arm_fails(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_sync_configure(&domain));
    TEST_ASSERT_EQUAL(BM_ERR_NOT_INIT, bm_sync_trigger(&domain));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sync_configure_arm_trigger);
    RUN_TEST(test_sync_trigger_before_arm_fails);
    return UNITY_END();
}
