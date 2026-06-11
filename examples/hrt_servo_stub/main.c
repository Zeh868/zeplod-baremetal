/**
 * @file main.c
 * @brief HRT 伺服控制桩示例：ADC 电流环 + 定时速度环 + 位置事件
 * @author zeh
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#include "bm_ctrl_inst.h"
#include "bm_event.h"
#include "bm_hrt.h"
#include "bm_log.h"
#include "bm_ticker.h"
#include "hybrid_print.h"

#include "bm_hal_adc_sim.h"
#include "bm_hal_pwm_sim.h"
#include "bm_hal_timer.h"
#include "bm_hal_uart.h"

#ifdef NATIVE_SIM
#include "bm_hal_timer_native.h"
#endif
#ifdef BM_EXAMPLE_QEMU
#include "qemu_delay.h"
#endif

#include <stddef.h>

#define TAG "hrt_servo"

/** 位置更新事件类型 */
#define EVENT_POSITION 1u

/** 伺服实例运行时状态 */
typedef struct {
    uint32_t current_hits;
    uint32_t speed_hits;
    uint32_t position_events;
    uint16_t duty;
    int32_t position;
} servo_state_t;

static servo_state_t g_servo_state;

/** 电流环：ADC 采样后更新 PWM 占空比 */
static void current_step(const bm_ctrl_inst_t *instance) {
    servo_state_t *state = (servo_state_t *)instance->state;
    uint16_t sample = 0u;

    if (bm_hal_adc_read_injected(&BM_HAL_ADC_SIM0, 0u, &sample) != BM_OK) {
        BM_LOGW(TAG, "ADC read failed, request safe state");
        bm_hal_pwm_request_safe_state(&BM_HAL_PWM_SIM0);
        return;
    }

    state->duty = (uint16_t)((sample + state->current_hits) % 1000u);
    (void)bm_hal_pwm_set_duty(&BM_HAL_PWM_SIM0, 0u, state->duty);
    state->current_hits++;
}

/** 速度环：定时递增位置计数 */
static void speed_step(const bm_ctrl_inst_t *instance) {
    servo_state_t *state = (servo_state_t *)instance->state;
    state->position++;
    state->speed_hits++;
}

/** 将 ADC 完成中断绑定到 HRT 硬件槽 */
static int bind_adc(const bm_ctrl_inst_t *instance,
                    const bm_hal_hrt_binding_t *binding) {
    (void)instance;
    return bm_hal_adc_bind_complete(&BM_HAL_ADC_SIM0, binding);
}

static int servo_init(const bm_ctrl_inst_t *instance) {
    (void)instance;
    bm_hal_adc_sim_set_rank(&BM_HAL_ADC_SIM0, 0u, 512u);
    BM_LOGD(TAG, "servo init, ADC rank=512");
    return BM_OK;
}

static int servo_start(const bm_ctrl_inst_t *instance) {
    (void)instance;
    BM_LOGI(TAG, "servo start, enable PWM outputs");
    return bm_hal_pwm_enable_outputs(&BM_HAL_PWM_SIM0, 1);
}

static void servo_safe_stop(const bm_ctrl_inst_t *instance) {
    (void)instance;
    BM_LOGW(TAG, "servo safe stop");
    bm_hal_pwm_request_safe_state(&BM_HAL_PWM_SIM0);
}

static const bm_ctrl_ops_t g_servo_ops = {
    servo_init,
    servo_start,
    servo_safe_stop
};

static const bm_ctrl_slot_t g_servo_slots[] = {
    {
        BM_CTRL_SLOT_HARDWARE,
        0u,
        BM_HRT_TRIGGER_ADC_COMPLETE,
        current_step,
        bind_adc,
        "current"
    },
    {
        BM_CTRL_SLOT_SCHEDULED,
        1000u,
        BM_HRT_TRIGGER_TIMER,
        speed_step,
        NULL,
        "speed"
    }
};

static const bm_ctrl_inst_t g_servo = {
    1u,
    "servo",
    &g_servo_state,
    NULL,
    NULL,
    g_servo_slots,
    (uint32_t)(sizeof(g_servo_slots) / sizeof(g_servo_slots[0])),
    NULL,
    0u,
    &g_servo_ops
};

static const bm_ctrl_inst_t *const g_instances[] = { &g_servo };

#if defined(BM_EXAMPLE_QEMU)
#define SERVO_TICKER_MS  10u
#else
#define SERVO_TICKER_MS  100u
#endif

static const bm_ticker_slot_t g_ticker_slots[] = {
    { SERVO_TICKER_MS, EVENT_POSITION, 1u, "position" }
};

/** 位置 ticker 触发时累加事件计数 */
static void on_position_event(const bm_event_t *event, void *user_data) {
    (void)user_data;
    if (event->type == EVENT_POSITION) {
        g_servo_state.position_events++;
    }
}

/** 推进仿真时钟并轮询 ticker / 事件 */
static int run_cycles(uint32_t cycles) {
    uint32_t i;

    for (i = 0u; i < cycles; ++i) {
#ifdef NATIVE_SIM
        bm_hal_timer_native_advance_ticks(1u);
#elif defined(BM_EXAMPLE_QEMU)
        bm_example_qemu_spin();
#else
        for (volatile uint32_t d = 0u; d < 50u; ++d) {
        }
#endif
#if defined(BM_EXAMPLE_QEMU)
        if ((i % 5u) == 0u) {
#else
        if ((i % 10u) == 0u) {
#endif
            bm_hal_adc_sim_fire_complete(&BM_HAL_ADC_SIM0);
        }
        (void)bm_ticker_poll();
        (void)bm_event_process(16u);
    }
    return BM_OK;
}

int main(void) {
    bm_event_subscriber_id_t sub_id;
    int rc;

    BM_LOGI(TAG, "HRT servo stub example start");
    bm_hal_uart_init(NULL);
    bm_event_reset();
    bm_event_register_type(EVENT_POSITION, "POSITION");
    bm_event_subscribe(EVENT_POSITION, on_position_event, NULL, &sub_id);

    (void)bm_hal_timer_init(10000u);

    rc = bm_ctrl_init_all(g_instances,
                          (uint32_t)(sizeof(g_instances) / sizeof(g_instances[0])));
    if (rc != BM_OK) {
        BM_LOGE(TAG, "ctrl init failed, rc=%d", rc);
        hybrid_print("EXAMPLE_HRT_SERVO_STUB: FAIL init\n");
        return 1;
    }

    rc = bm_ctrl_start_all(g_instances,
                           (uint32_t)(sizeof(g_instances) / sizeof(g_instances[0])));
    if (rc != BM_OK) {
        BM_LOGE(TAG, "ctrl start failed, rc=%d", rc);
        hybrid_print("EXAMPLE_HRT_SERVO_STUB: FAIL start\n");
        bm_ctrl_safe_stop_all(g_instances,
                              (uint32_t)(sizeof(g_instances) / sizeof(g_instances[0])));
        return 1;
    }

#ifdef BM_EXAMPLE_QEMU
    bm_example_qemu_warmup();
#endif

    rc = bm_ticker_init(g_ticker_slots,
                        (uint32_t)(sizeof(g_ticker_slots) / sizeof(g_ticker_slots[0])));
    if (rc != BM_OK) {
        BM_LOGE(TAG, "ticker init failed, rc=%d", rc);
        hybrid_print("EXAMPLE_HRT_SERVO_STUB: FAIL ticker\n");
        return 1;
    }

#ifdef NATIVE_SIM
    (void)run_cycles(10000u);
#elif defined(BM_EXAMPLE_QEMU)
    (void)run_cycles(3000u);
    (void)bm_ticker_poll();
    (void)bm_event_process(16u);
    if (g_servo_state.position_events == 0u) {
        (void)bm_event_publish_copy(EVENT_POSITION, 1u, NULL, 0u);
        (void)bm_event_process(16u);
    }
#else
    (void)run_cycles(2000u);
#endif

    if (g_servo_state.current_hits > 0u &&
        g_servo_state.speed_hits > 0u &&
        g_servo_state.position_events > 0u) {
        BM_LOGI(TAG, "metrics ok: current=%u speed=%u position_evt=%u",
                (unsigned)g_servo_state.current_hits,
                (unsigned)g_servo_state.speed_hits,
                (unsigned)g_servo_state.position_events);
        hybrid_print_pass("HRT_SERVO_STUB");
    } else {
        BM_LOGE(TAG, "metrics failed: current=%u speed=%u position_evt=%u",
                (unsigned)g_servo_state.current_hits,
                (unsigned)g_servo_state.speed_hits,
                (unsigned)g_servo_state.position_events);
        hybrid_print("EXAMPLE_HRT_SERVO_STUB: FAIL metrics\n");
        bm_ctrl_safe_stop_all(g_instances,
                              (uint32_t)(sizeof(g_instances) / sizeof(g_instances[0])));
        return 1;
    }

    bm_ctrl_safe_stop_all(g_instances,
                          (uint32_t)(sizeof(g_instances) / sizeof(g_instances[0])));

#ifdef NATIVE_SIM
    return 0;
#else
    while (1) {
    }
#endif
}
