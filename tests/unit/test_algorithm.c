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

static const bm_algo_goertzel_config_t s_readonly_goertzel_config = {
    .target_freq_hz = 100.0f,
    .sample_hz = 1000.0f,
    .block_size = 20u,
    .coeff = 0.0f
};

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

static void test_hpf1_uses_high_pass_coefficient(void) {
    bm_algo_hpf1_config_t cfg;
    bm_algo_hpf1_state_t st;
    float first;
    float settled = 0.0f;
    int i;

    TEST_ASSERT_EQUAL(0, bm_algo_hpf1_init_from_cutoff(&cfg, 10.0f, 1000.0f));
    bm_algo_hpf1_reset(&st);
    first = bm_algo_hpf1_step(&st, &cfg, 1.0f);
    for (i = 0; i < 200; ++i) {
        settled = bm_algo_hpf1_step(&st, &cfg, 1.0f);
    }

    TEST_ASSERT_TRUE(first > 0.9f);
    TEST_ASSERT_TRUE(fabsf(settled) < 0.001f);
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

static void test_scurve_reaches_target(void) {
    bm_algo_scurve_config_t cfg = {
        .max_vel = 1.0f,
        .max_accel = 2.0f,
        .max_jerk = 10.0f
    };
    bm_algo_scurve_state_t st;
    int i;

    bm_algo_scurve_reset(&st, 0.0f, 0.0f, 0.0f);
    bm_algo_scurve_set_target(&st, 1.0f);
    for (i = 0; i < 1000 && !st.done; ++i) {
        (void)bm_algo_scurve_step(&st, &cfg, 0.01f);
    }

    TEST_ASSERT_EQUAL(1, st.done);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, st.position);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, st.velocity);
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

static void test_single_point_windows_are_finite(void) {
    float window;

    bm_algo_window_hann(&window, 1u);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 1.0f, window);
    bm_algo_window_hamming(&window, 1u);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 1.0f, window);
    bm_algo_window_blackman(&window, 1u);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 1.0f, window);
}

static void test_image_label_merges_connected_pixels(void) {
    const uint8_t binary[12] = {
        1u, 1u, 0u, 0u,
        0u, 1u, 0u, 0u,
        0u, 0u, 0u, 1u
    };
    uint16_t labels[12];
    bm_algo_blob_info_t blobs[2];
    int count;

    count = bm_algo_image_label_u8(binary, labels, 4u, 3u, blobs, 2u);

    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL(labels[0], labels[1]);
    TEST_ASSERT_EQUAL(labels[0], labels[5]);
    TEST_ASSERT_NOT_EQUAL(labels[0], labels[11]);
    TEST_ASSERT_EQUAL_UINT32(3u, blobs[0].area);
    TEST_ASSERT_EQUAL_UINT32(1u, blobs[1].area);
}

static void test_linear_resampler_ratio_and_capacity(void) {
    bm_algo_linear_resampler_state_t st;
    float outputs[3];
    uint32_t count;

    bm_algo_linear_resampler_reset(&st, 2.0f, 0.0f);
    TEST_ASSERT_EQUAL(2, bm_algo_linear_resampler_step(
        &st, 1.0f, outputs, 3u, &count));
    TEST_ASSERT_EQUAL_UINT32(2u, count);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, outputs[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, outputs[1]);

    bm_algo_linear_resampler_reset(&st, 0.5f, 0.0f);
    TEST_ASSERT_EQUAL(0, bm_algo_linear_resampler_step(
        &st, 1.0f, outputs, 3u, &count));
    TEST_ASSERT_EQUAL(1, bm_algo_linear_resampler_step(
        &st, 2.0f, outputs, 3u, &count));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, outputs[0]);

    bm_algo_linear_resampler_reset(&st, 3.0f, 0.0f);
    TEST_ASSERT_EQUAL(-1, bm_algo_linear_resampler_step(
        &st, 1.0f, outputs, 2u, &count));
    TEST_ASSERT_EQUAL_UINT32(0u, count);
}

static void test_goertzel_accepts_readonly_config(void) {
    bm_algo_goertzel_state_t st;

    TEST_ASSERT_EQUAL(0, bm_algo_goertzel_init(
        &st, &s_readonly_goertzel_config));
    TEST_ASSERT_TRUE(fabsf(st.coeff) > 0.1f);
    TEST_ASSERT_FLOAT_WITHIN(
        0.0f, 0.0f, s_readonly_goertzel_config.coeff);
}

static void test_ekf_covariance_stays_symmetric(void) {
    bm_algo_ekf_cv_config_t cfg = {
        .q_pos = 0.01f,
        .q_vel = 0.01f,
        .r_pos = 0.1f
    };
    bm_algo_ekf_cv_state_t st;
    int i;

    bm_algo_ekf_cv_reset(&st, 0.0f, 0.0f);
    for (i = 0; i < 20; ++i) {
        bm_algo_ekf_cv_predict(&st, &cfg, 0.1f);
        bm_algo_ekf_cv_update(&st, &cfg, 1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, st.p01, st.p10);
}

static void test_mahony_uses_simultaneous_quaternion_update(void) {
    bm_algo_mahony_config_t cfg = { .kp = 0.0f, .ki = 0.0f };
    bm_algo_mahony_state_t st;
    float norm = sqrtf(1.14f);

    bm_algo_mahony_reset(&st);
    bm_algo_mahony_step(&st, &cfg, 1.0f, 2.0f, 3.0f,
                        0.0f, 0.0f, 0.0f, 0.2f);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f / norm, st.q.w);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.1f / norm, st.q.x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.2f / norm, st.q.y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.3f / norm, st.q.z);
}

static void test_sogi_states_decay_after_input_stops(void) {
    bm_algo_sogi_pll_config_t cfg = {
        .nominal_omega_rad_s = 2.0f * 3.14159265f * 50.0f,
        .k_sogi = 1.41421356f,
        .k_pll = 0.0f
    };
    bm_algo_sogi_pll_state_t st;
    int i;

    bm_algo_sogi_pll_reset(&st, &cfg);
    for (i = 0; i < 2000; ++i) {
        float input = sinf(cfg.nominal_omega_rad_s * (float)i * 0.0001f);
        bm_algo_sogi_pll_step(&st, &cfg, input, 0.0001f);
    }
    for (i = 0; i < 2000; ++i) {
        bm_algo_sogi_pll_step(&st, &cfg, 0.0f, 0.0001f);
    }

    TEST_ASSERT_TRUE(fabsf(st.v_alpha) < 0.001f);
    TEST_ASSERT_TRUE(fabsf(st.v_beta) < 0.001f);
}

static void test_fixed_pi_q31_saturates(void) {
    bm_algo_pi_q31_config_t cfg = {
        .kp = bm_algo_float_to_q31(0.8f),
        .ki = bm_algo_float_to_q31(0.5f),
        .out_min = bm_algo_float_to_q31(-1.0f),
        .out_max = bm_algo_float_to_q31(1.0f),
        .integrator_min = bm_algo_float_to_q31(-1.0f),
        .integrator_max = bm_algo_float_to_q31(1.0f)
    };
    bm_algo_pi_q31_state_t st;
    bm_algo_q31_t out;
    int i;

    bm_algo_pi_q31_reset(&st, 0);
    for (i = 0; i < 300; ++i) {
        out = bm_algo_pi_q31_step(&st, &cfg,
                                  bm_algo_float_to_q31(1.0f),
                                  bm_algo_float_to_q31(0.01f));
    }
    TEST_ASSERT_FLOAT_WITHIN(0.08f, 1.0f, bm_algo_q31_to_float(out));
}

static void test_fixed_lpf1_q15_tracks_input(void) {
    bm_algo_lpf1_q15_config_t cfg = {
        .alpha_q15 = bm_algo_float_to_q15(0.1f)
    };
    bm_algo_lpf1_q15_state_t st;
    bm_algo_q15_t v = 0;
    int i;

    bm_algo_lpf1_q15_reset(&st, 0);
    for (i = 0; i < 500; ++i) {
        v = bm_algo_lpf1_q15_step(&st, &cfg, BM_ALGO_Q15_ONE);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, bm_algo_q15_to_float(v));
}

static void test_fixed_point_saturates_before_narrowing(void) {
    bm_algo_biquad_q15_config_t bq_cfg = {
        .b0 = BM_ALGO_Q15_ONE,
        .b1 = 0,
        .b2 = 0,
        .a1 = 0,
        .a2 = 0
    };
    bm_algo_biquad_q15_state_t bq = {
        .z1 = BM_ALGO_Q15_ONE,
        .z2 = 0
    };
    bm_algo_pi_q31_config_t pi_cfg = {
        .kp = 0,
        .ki = 0,
        .out_min = (bm_algo_q31_t)INT32_MIN,
        .out_max = BM_ALGO_Q31_ONE,
        .integrator_min = (bm_algo_q31_t)INT32_MIN,
        .integrator_max = BM_ALGO_Q31_ONE
    };
    bm_algo_pi_q31_state_t pi = {
        .integrator = BM_ALGO_Q31_ONE - 1,
        .output = 0
    };

    TEST_ASSERT_EQUAL_INT16(
        BM_ALGO_Q15_ONE,
        bm_algo_biquad_q15_step(&bq, &bq_cfg, BM_ALGO_Q15_ONE));
    (void)bm_algo_pi_q31_step(
        &pi, &pi_cfg, BM_ALGO_Q31_ONE, bm_algo_float_to_q31(0.5f));
    TEST_ASSERT_EQUAL_INT32(BM_ALGO_Q31_ONE, pi.integrator);
}

static void test_flux_observer_and_mtpa(void) {
    bm_algo_flux_observer_config_t obs_cfg_ls = {
        .rs_ohm = 0.5f, .ls_h = 0.001f, .pll_kp = 10.0f, .pll_ki = 100.0f
    };
    bm_algo_flux_observer_config_t obs_cfg_no_ls = obs_cfg_ls;
    bm_algo_flux_observer_state_t obs_ls;
    bm_algo_flux_observer_state_t obs_no_ls;
    float theta_ls;
    float theta_no_ls;
    int i;

    obs_cfg_no_ls.ls_h = 0.0f;
    bm_algo_flux_observer_reset(&obs_ls, 0.0f);
    bm_algo_flux_observer_reset(&obs_no_ls, 0.0f);
    for (i = 0; i < 100; ++i) {
        float t = (float)i * 0.001f;
        float v_alpha = -sinf(t);
        float v_beta = cosf(t);
        float i_alpha = 0.5f * sinf(t);
        float i_beta = -0.5f * cosf(t);

        theta_ls = bm_algo_flux_observer_step(&obs_ls, &obs_cfg_ls,
                                              v_alpha, v_beta,
                                              i_alpha, i_beta, 0.001f);
        theta_no_ls = bm_algo_flux_observer_step(&obs_no_ls, &obs_cfg_no_ls,
                                                   v_alpha, v_beta,
                                                   i_alpha, i_beta, 0.001f);
        (void)theta_ls;
        (void)theta_no_ls;
    }
    TEST_ASSERT_TRUE(fabsf(obs_ls.omega_rad_s) > 0.1f);
    TEST_ASSERT_TRUE(fabsf(obs_ls.theta_rad - obs_no_ls.theta_rad) > 0.01f);
    TEST_ASSERT_TRUE(bm_algo_mtpa_id_ref(2.0f, 0.001f, 0.002f, 0.05f) < 0.0f);
    TEST_ASSERT_TRUE(
        fabsf(bm_algo_mtpa_id_ref(2.0f, 0.001f, 0.002f, 0.01f)) >
        fabsf(bm_algo_mtpa_id_ref(2.0f, 0.001f, 0.002f, 0.20f)));
}

static void test_battery_temp_and_motor_extras(void) {
    bm_algo_battery_temp_config_t temp_cfg = {
        .ref_temp_c = 25.0f,
        .capacity_coeff_per_c = 0.01f,
        .ocv_shift_v_per_c = 0.002f
    };
    bm_algo_abc_t abc;
    bm_algo_rate_est_state_t rate;

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.1f,
        bm_algo_battery_temp_capacity_ah(1.0f, 35.0f, &temp_cfg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.68f,
        bm_algo_battery_temp_compensate_ocv(3.7f, 35.0f, &temp_cfg));

    bm_algo_current_from_2shunt(1.0f, -0.5f, &abc);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, abc.ic);

    bm_algo_rate_est_reset(&rate, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f,
        bm_algo_rate_est_step(&rate, 1.0f, 0.1f));
}

static void test_zero_length_audio_is_ignored(void) {
    bm_algo_agc_config_t agc_cfg = {
        .target_level = 1.0f,
        .attack_coeff = 1.0f,
        .release_coeff = 1.0f,
        .gain = 1.0f
    };
    bm_algo_vad_config_t vad_cfg = {
        .energy_threshold = 0.1f,
        .alpha = 1.0f
    };
    bm_algo_agc_state_t agc;
    bm_algo_vad_state_t vad;
    float sample = 1.0f;

    bm_algo_agc_reset(&agc, 2.0f);
    bm_algo_vad_reset(&vad);
    bm_algo_agc_process(&agc, &agc_cfg, &sample, &sample, 0u);
    bm_algo_vad_process(&vad, &vad_cfg, &sample, 0u);

    TEST_ASSERT_FLOAT_WITHIN(0.0f, 2.0f, agc.gain);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 0.0f, vad.energy);
}

static void test_vision_centroid_and_compensation(void) {
    const uint8_t mask[9] = {
        0u, 1u, 0u,
        1u, 1u, 0u,
        0u, 0u, 0u
    };
    float cx = 0.0f;
    float cy = 0.0f;

    TEST_ASSERT_EQUAL(0, bm_algo_vision_centroid_u8(mask, 3u, 3u, &cx, &cy));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.667f, cx);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.667f, cy);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f,
        bm_algo_deadzone_inverse(0.05f, 0.1f, 2.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.2f,
        bm_algo_deadzone_inverse(0.2f, 0.1f, 2.0f));
}

static void test_soc_ekf_and_power_quality(void) {
    bm_algo_soc_ekf_config_t ekf_cfg = {
        .q_soc = 1e-6f,
        .q_bias = 1e-8f,
        .r_v = 0.01f,
        .coulomb_efficiency = 1.0f,
        .nominal_capacity_ah = 10.0f,
        .ocv_slope_v_per_soc = 0.5f
    };
    bm_algo_soc_ekf_state_t ekf;
    const float harmonics[3] = { 1.0f, 0.1f, 0.05f };
    float p;
    float q;
    float s;

    bm_algo_soc_ekf_reset(&ekf, 0.5f);
    bm_algo_soc_ekf_predict(&ekf, &ekf_cfg, 1.0f, 1.0f);
    bm_algo_soc_ekf_update_voltage(&ekf, &ekf_cfg, 3.7f, 3.65f);
    TEST_ASSERT_TRUE(ekf.soc >= 0.0f && ekf.soc <= 1.0f);
    TEST_ASSERT_TRUE(fabsf(ekf.p10) > 0.0f);
    TEST_ASSERT_TRUE(fabsf(ekf.bias_a) > 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.5f, 11.18f,
        bm_algo_thd_percent(harmonics, 3u));
    bm_algo_power_quality_pq(230.0f, 10.0f, 0.0f, &p, &q, &s);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 2300.0f, p);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, q);
}

static void test_detection_matched_and_ultrasonic(void) {
    const float sig[8] = { 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    const float tmpl[3] = { 1.0f, 2.0f, 1.0f };
    uint32_t idx = 0u;
    float echo[32];
    int32_t tof;
    int i;

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f,
        bm_algo_matched_filter(sig, 8u, tmpl, 3u, &idx));
    TEST_ASSERT_EQUAL_UINT32(2u, idx);

    for (i = 0; i < 32; ++i) {
        echo[i] = 0.0f;
    }
    echo[10] = 1.0f;
    echo[11] = 0.8f;
    tof = bm_algo_ultrasonic_tof(echo, 32u, 4u, 0.3f, 0.5f);
    TEST_ASSERT_EQUAL_INT32(10, tof);
}

static void test_matched_filter_accepts_negative_correlations(void) {
    const float signal[2] = { -2.0f, -3.0f };
    const float template_data[1] = { 1.0f };
    uint32_t index = UINT32_MAX;

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, -2.0f,
        bm_algo_matched_filter(signal, 2u, template_data, 1u, &index));
    TEST_ASSERT_EQUAL_UINT32(0u, index);
}

static void test_w2_audio_spectral_motion(void) {
    bm_algo_eq_peaking_config_t eq_cfg = {
        .sample_hz = 48000.0f,
        .freq_hz = 1000.0f,
        .q = 1.0f,
        .gain_db = 6.0f
    };
    bm_algo_eq_peaking_state_t eq;
    float in[4] = { 1.0f, 0.0f, -1.0f, 0.0f };
    float out[4];
    float win[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float mag[4];
    bm_algo_stepper_config_t st_cfg = { .max_velocity_steps_s = 1000.0f };
    bm_algo_stepper_state_t st;
    int8_t pulses[4];

    TEST_ASSERT_EQUAL(0, bm_algo_eq_peaking_design(&eq, &eq_cfg));
    bm_algo_eq_peaking_process(&eq, &eq_cfg, in, out, 4u);
    TEST_ASSERT_TRUE(fabsf(out[0]) > 0.0f);

    TEST_ASSERT_EQUAL(0, bm_algo_stft_magnitude_frame(in, win, 4u, mag));
    TEST_ASSERT_TRUE(mag[0] >= 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f,
        bm_algo_order_from_hz(50.0f, 3000.0f, 1u));

    bm_algo_stepper_reset(&st, 0);
    TEST_ASSERT_TRUE(bm_algo_stepper_process(&st, &st_cfg, 100.0f, 0.01f,
                                             pulses, 4u) > 0u);
}

static void test_eq_stepper_and_smith_regressions(void) {
    bm_algo_eq_peaking_config_t eq_cfg = {
        .sample_hz = 48000.0f,
        .freq_hz = 1000.0f,
        .q = 1.0f,
        .gain_db = 0.0f
    };
    bm_algo_eq_peaking_state_t eq;
    const float impulse[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    float eq_out[4];
    bm_algo_stepper_config_t step_cfg = {
        .max_velocity_steps_s = 1000.0f
    };
    bm_algo_stepper_state_t step;
    int8_t pulse;
    bm_algo_smith_predictor_config_t smith_cfg = {
        .model_gain = 1.0f,
        .delay_steps = 1u
    };
    bm_algo_smith_predictor_state_t smith;
    float delay_line[1];
    float low_measurement;
    float high_measurement;

    TEST_ASSERT_EQUAL(0, bm_algo_eq_peaking_design(&eq, &eq_cfg));
    bm_algo_eq_peaking_process(&eq, &eq_cfg, impulse, eq_out, 4u);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, eq_out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, eq_out[1]);

    bm_algo_stepper_reset(&step, 0);
    TEST_ASSERT_EQUAL_UINT32(
        1u,
        bm_algo_stepper_process(
            &step, &step_cfg, 250.0f, 0.01f, &pulse, 1u));
    TEST_ASSERT_EQUAL_INT32(1, step.position_steps);
    TEST_ASSERT_TRUE(step.phase >= 1.0f);

    TEST_ASSERT_EQUAL(
        0,
        bm_algo_smith_predictor_init(
            &smith, &smith_cfg, delay_line, 1u));
    low_measurement = bm_algo_smith_predictor_step(
        &smith, &smith_cfg, 5.0f, 2.0f, 1.0f);
    bm_algo_smith_predictor_reset(&smith, &smith_cfg);
    high_measurement = bm_algo_smith_predictor_step(
        &smith, &smith_cfg, 5.0f, 3.0f, 1.0f);
    TEST_ASSERT_TRUE(high_measurement < low_measurement);
}

static void test_review_fixes(void) {
    bm_algo_eq_peaking_config_t bad_eq = { .sample_hz = 0.0f };
    bm_algo_eq_peaking_state_t eq;
    float in[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float out[4];
    float ref[64];
    float sig[64];
    float gcc_work[512];
    bm_algo_smith_predictor_config_t smith_cfg = {
        .model_gain = 1.0f,
        .delay_steps = 2u
    };
    float delay_line[2];
    bm_algo_smith_predictor_state_t smith;
    const uint8_t src[9] = { 0u };
    int16_t gx[9];
    int16_t gy[9];
    int i;

    memset(&eq, 0, sizeof(eq));
    bm_algo_eq_peaking_process(&eq, &bad_eq, in, out, 4u);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, out[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, out[3]);

    for (i = 0; i < 64; ++i) {
        ref[i] = 0.0f;
        sig[i] = 0.0f;
    }
    ref[10] = 1.0f;
    sig[13] = 1.0f;
    TEST_ASSERT_EQUAL_UINT32(512u, bm_algo_gcc_phat_work_count(64u, 10));
    TEST_ASSERT_EQUAL_INT32(3, bm_algo_gcc_phat_delay(ref, sig, 64u, 10,
                                                      gcc_work, 512u));
    TEST_ASSERT_EQUAL_INT32(BM_ALGO_GCC_PHAT_DELAY_INVALID,
                            bm_algo_gcc_phat_delay(NULL, sig, 64u, 10,
                                                   gcc_work, 512u));

    TEST_ASSERT_EQUAL(0, bm_algo_smith_predictor_init(&smith, &smith_cfg,
                                                      delay_line, 2u));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f,
        bm_algo_smith_predictor_step(&smith, &smith_cfg, 5.0f, 2.0f, 1.0f));

    memset(gx, 0x5A, sizeof(gx));
    memset(gy, 0x5A, sizeof(gy));
    bm_algo_vision_sobel_u8(src, gx, gy, 3u, 3u);
    TEST_ASSERT_EQUAL_INT16(0, gx[0]);
    TEST_ASSERT_EQUAL_INT16(0, gy[8]);
}

void test_algorithm(void) {
    RUN_TEST(test_common_clamp_and_deadband);
    RUN_TEST(test_pi_step_and_saturation);
    RUN_TEST(test_lpf1_step);
    RUN_TEST(test_hpf1_uses_high_pass_coefficient);
    RUN_TEST(test_ramp_reaches_target);
    RUN_TEST(test_scurve_reaches_target);
    RUN_TEST(test_clarke_park_roundtrip);
    RUN_TEST(test_coulomb_soc);
    RUN_TEST(test_rfft_execute);
    RUN_TEST(test_single_point_windows_are_finite);
    RUN_TEST(test_image_label_merges_connected_pixels);
    RUN_TEST(test_linear_resampler_ratio_and_capacity);
    RUN_TEST(test_goertzel_accepts_readonly_config);
    RUN_TEST(test_ekf_covariance_stays_symmetric);
    RUN_TEST(test_mahony_uses_simultaneous_quaternion_update);
    RUN_TEST(test_sogi_states_decay_after_input_stops);
    RUN_TEST(test_fixed_pi_q31_saturates);
    RUN_TEST(test_fixed_lpf1_q15_tracks_input);
    RUN_TEST(test_fixed_point_saturates_before_narrowing);
    RUN_TEST(test_flux_observer_and_mtpa);
    RUN_TEST(test_battery_temp_and_motor_extras);
    RUN_TEST(test_zero_length_audio_is_ignored);
    RUN_TEST(test_vision_centroid_and_compensation);
    RUN_TEST(test_soc_ekf_and_power_quality);
    RUN_TEST(test_detection_matched_and_ultrasonic);
    RUN_TEST(test_matched_filter_accepts_negative_correlations);
    RUN_TEST(test_w2_audio_spectral_motion);
    RUN_TEST(test_eq_stepper_and_smith_regressions);
    RUN_TEST(test_review_fixes);
}

int main(void) {
    UNITY_BEGIN();
    test_algorithm();
    return UNITY_END();
}
