/**
 * @file test_algorithm.c
 * @brief bm_algorithm 算法库单元测试
 *
 * 覆盖公共工具、PI、滤波、斜坡、电机变换、FFT 与电池算法基本行为。
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       1.0            zeh            正式发布
 */
#include "unity.h"
#include "bm_algorithm.h"

#include <math.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_common_clamp_and_deadband(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, bm_algo_clamp_f(10.0f, 0.0f, 5.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, bm_algo_deadband_f(0.05f, 0.1f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, bm_algo_deadband_f(0.2f, 0.1f));
}

static void test_pi_step_and_saturation(void) {
    bm_algo_pi_config_t cfg = {
        .kp = 1.0f,
        .ki = 10.0f,
        .out_min = -1.0f,
        .out_max = 1.0f,
        .integrator_min = -10.0f,
        .integrator_max = 10.0f
    };
    bm_algo_pi_state_t st;
    float out;
    int i;

    bm_algo_pi_reset(&st, 0.0f);
    for (i = 0; i < 100; ++i) {
        out = bm_algo_pi_step(&st, &cfg, 1.0f, 0.001f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, out);
}

static void test_lpf1_step(void) {
    bm_algo_lpf1_config_t cfg;
    bm_algo_lpf1_state_t st;
    float v = 0.0f;
    int i;

    TEST_ASSERT_EQUAL(0, bm_algo_lpf1_init_from_cutoff(&cfg, 10.0f, 1000.0f));
    bm_algo_lpf1_reset(&st, 0.0f);
    for (i = 0; i < 500; ++i) {
        v = bm_algo_lpf1_step(&st, &cfg, 1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, v);
}

static void test_ramp_reaches_target(void) {
    bm_algo_ramp_config_t cfg = { .rate_per_s = 10.0f };
    bm_algo_ramp_state_t st;
    float v;
    int i;

    bm_algo_ramp_reset(&st, 0.0f);
    for (i = 0; i < 200; ++i) {
        v = bm_algo_ramp_step(&st, &cfg, 1.0f, 0.01f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, v);
    TEST_ASSERT_EQUAL(1, st.done);
}

static void test_clarke_park_roundtrip(void) {
    bm_algo_abc_t abc = { .ia = 1.0f, .ib = -0.5f, .ic = -0.5f };
    bm_algo_alphabeta_t ab;
    bm_algo_dq_t dq;
    bm_algo_alphabeta_t ab2;

    bm_algo_clarke(&abc, &ab);
    bm_algo_park(&ab, 0.5f, &dq);
    bm_algo_inv_park(&dq, 0.5f, &ab2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, ab.i_alpha, ab2.i_alpha);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, ab.i_beta, ab2.i_beta);
}

static void test_coulomb_soc(void) {
    bm_algo_coulomb_config_t cfg = {
        .nominal_capacity_ah = 10.0f,
        .coulomb_efficiency = 1.0f,
        .soc_min = 0.0f,
        .soc_max = 1.0f
    };
    bm_algo_coulomb_state_t st;
    float soc;

    bm_algo_coulomb_reset(&st, 0.5f);
    soc = bm_algo_coulomb_step(&st, &cfg, 1.0f, 3600.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.6f, soc);
}

static void test_rfft_execute(void) {
    float time_data[BM_ALGO_FFT_SIZE_64];
    float work[BM_ALGO_FFT_SIZE_64 * 2u];
    float spectrum[BM_ALGO_FFT_SIZE_64 / 2u + 1u];
    bm_algo_rfft_f32_t fft;
    uint32_t i;

    for (i = 0u; i < BM_ALGO_FFT_SIZE_64; ++i) {
        time_data[i] = sinf(2.0f * 3.14159265f * 4.0f * (float)i / (float)BM_ALGO_FFT_SIZE_64);
    }

    TEST_ASSERT_EQUAL(0, bm_algo_rfft_f32_init(&fft, BM_ALGO_FFT_SIZE_64, work, (uint32_t)(sizeof(work) / sizeof(work[0]))));
    TEST_ASSERT_EQUAL(0, bm_algo_rfft_f32_execute(&fft, time_data, spectrum));
    TEST_ASSERT_TRUE(spectrum[4] > spectrum[3]);
    TEST_ASSERT_TRUE(spectrum[4] > spectrum[5]);
}

void test_algorithm(void) {
    RUN_TEST(test_common_clamp_and_deadband);
    RUN_TEST(test_pi_step_and_saturation);
    RUN_TEST(test_lpf1_step);
    RUN_TEST(test_ramp_reaches_target);
    RUN_TEST(test_clarke_park_roundtrip);
    RUN_TEST(test_coulomb_soc);
    RUN_TEST(test_rfft_execute);
}

int main(void) {
    UNITY_BEGIN();
    test_algorithm();
    return UNITY_END();
}
