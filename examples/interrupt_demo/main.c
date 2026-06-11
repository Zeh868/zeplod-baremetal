/**
 * @file main.c
 * @brief 中断驱动示例：SysTick + TIMER1 ISR 发布事件
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#include "bm_core.h"
#include "bm_hal_timer.h"
#include "bm_hal_uart.h"
#include "bm_log.h"
#include "example_support.h"
#include "interrupt_timer.h"

#define TAG "irq_demo"

#define EVENT_TICK    1
#define EVENT_SAMPLE  2
#define EVENT_STATS   3

/** SysTick 频率（Hz） */
#define SYSTICK_FREQ_HZ 10U

static volatile uint32_t tick_count;
static volatile uint32_t sample_count;
static volatile uint16_t next_temperature = 25;

/** 在 ISR 中发布 tick 事件 */
static void publish_tick_from_isr(void) {
    uint32_t tick = tick_count + 1U;
    if (bm_event_publish_copy_from_isr(
            EVENT_TICK, 0, &tick, sizeof(tick)) == BM_OK) {
        tick_count = tick;
    }
}

/** 在 ISR 中发布温度采样事件 */
static void publish_sample_from_isr(void) {
    uint16_t temperature = next_temperature++;
    if (bm_event_publish_copy_from_isr(
            EVENT_SAMPLE, 1, &temperature, sizeof(temperature)) == BM_OK) {
        sample_count++;
    }
}

static void tick_on_event(const bm_event_t *event, void *user_data) {
    const uint32_t *tick = event->data;
    (void)user_data;

    example_print("[tick] count=");
    example_print_u32(*tick);
    example_print("\n");
}

static void sample_on_event(const bm_event_t *event, void *user_data) {
    const uint16_t *temperature = event->data;
    (void)user_data;

    example_print("[sample] temperature=");
    example_print_u32(*temperature);
    example_print("\n");
}

static void stats_on_event(const bm_event_t *event, void *user_data) {
    (void)event;
    (void)user_data;

    example_print("[stats] ticks=");
    example_print_u32(tick_count);
    example_print(", samples=");
    example_print_u32(sample_count);
    example_print("\n");
}

static int app_init(void) {
    bm_event_subscriber_id_t subscriber_id;

    bm_event_reset();
    if (bm_event_register_type(EVENT_TICK, "TICK") != BM_OK ||
        bm_event_register_type(EVENT_SAMPLE, "SAMPLE") != BM_OK ||
        bm_event_register_type(EVENT_STATS, "STATS") != BM_OK ||
        bm_event_subscribe(EVENT_TICK, tick_on_event, NULL,
                           &subscriber_id) != BM_OK ||
        bm_event_subscribe(EVENT_SAMPLE, sample_on_event, NULL,
                           &subscriber_id) != BM_OK ||
        bm_event_subscribe(EVENT_STATS, stats_on_event, NULL,
                           &subscriber_id) != BM_OK) {
        BM_LOGE(TAG, "event setup failed");
        return BM_ERR_INVALID;
    }

    bm_hal_timer_set_callback(publish_tick_from_isr);
    if (bm_hal_timer_init(SYSTICK_FREQ_HZ) != 0) {
        BM_LOGE(TAG, "systick init failed");
        return BM_ERR_NOT_INIT;
    }
    interrupt_timer_init(publish_sample_from_isr);
    BM_LOGI(TAG, "app init ok, systick=%u Hz", (unsigned)SYSTICK_FREQ_HZ);
    return BM_OK;
}

int main(void) {
    uint8_t stats_sent = 0;
    uint8_t pass_sent = 0;

    bm_hal_uart_init(NULL);
    example_print("Zeplod Example: interrupt_demo\n");
    BM_LOGI(TAG, "interrupt_demo example start");

    if (app_init() != BM_OK) {
        BM_LOGE(TAG, "app init failed");
        example_print("EXAMPLE_IRQ: INIT FAILED\n");
        return 1;
    }

    while (1) {
        bm_event_process(8);

        if (tick_count >= 20U && sample_count >= 2U && !stats_sent) {
            if (bm_event_publish_copy(EVENT_STATS, 2, NULL, 0) == BM_OK) {
                BM_LOGI(TAG, "stats event published: ticks=%u samples=%u",
                        (unsigned)tick_count, (unsigned)sample_count);
                stats_sent = 1;
            }
        }

        if (stats_sent && sample_count >= 2U && !pass_sent) {
            BM_LOGI(TAG, "example PASS");
            example_print("EXAMPLE_IRQ: PASS\n");
            pass_sent = 1;
        }
    }
}
