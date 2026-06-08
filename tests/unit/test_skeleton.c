#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_skeleton_passes(void) {
    TEST_ASSERT_EQUAL(1, 1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_skeleton_passes);
    return UNITY_END();
}
