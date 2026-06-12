/**
 * @file main.c
 * @brief 全系统集成示例：模块、事件风暴优先级、看门狗与故障处理
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
#include "bm_hal_uart.h"
#include "bm_log.h"
#include "bm_module.h"
#include "bm_wdg.h"
#include "example_support.h"

#define TAG "full_system"

#define EVENT_TEMP          1
#define EVENT_SENSOR_FAULT  2
#define EVENT_STORM_BASE    8
#define EVENT_STORM_COUNT   5

typedef struct {
    uint16_t temp;
    uint16_t humidity;
    uint32_t timestamp;
    uint8_t status;
} sensor_data_t;

BM_MEMPOOL_DEFINE(sensor_pool, sensor_data_t, 4);

static uint32_t sensor_cycle;
static uint8_t fault_observed;
static uint8_t storm_priorities[EVENT_STORM_COUNT];
static uint8_t storm_event_count;

static void display_on_temp(const bm_event_t *event, void *user_data) {
    const sensor_data_t *sample = event->data;
    (void)user_data;

    example_print("[display] temperature=");
    example_print_u32(sample->temp);
    example_print(", humidity=");
    example_print_u32(sample->humidity);
    example_print("\n");
}

static void comm_on_temp(const bm_event_t *event, void *user_data) {
    (void)event;
    (void)user_data;
    example_print("[comm] sensor frame sent\n");
}

static void fault_on_event(const bm_event_t *event, void *user_data) {
    const uint8_t *code = event->data;
    (void)user_data;

    example_print("[fault] sensor code=");
    example_print_u32(*code);
    example_print("\n");
    BM_LOGW(TAG, "sensor fault code=%u", (unsigned)*code);
    fault_observed = 1;
}

/** 记录事件风暴中各事件的实际处理优先级 */
static void storm_on_event(const bm_event_t *event, void *user_data) {
    (void)user_data;
    if (storm_event_count < EVENT_STORM_COUNT) {
        storm_priorities[storm_event_count++] = event->priority;
    }
}

static int register_events(void) {
    bm_event_subscriber_id_t subscriber_id;

    if (bm_event_register_type(EVENT_TEMP, "TEMP") != BM_OK ||
        bm_event_register_type(EVENT_SENSOR_FAULT, "SENSOR_FAULT") != BM_OK ||
        bm_event_subscribe(EVENT_TEMP, display_on_temp, NULL, &subscriber_id) != BM_OK ||
        bm_event_subscribe(EVENT_TEMP, comm_on_temp, NULL, &subscriber_id) != BM_OK ||
        bm_event_subscribe(EVENT_SENSOR_FAULT, fault_on_event, NULL,
                           &subscriber_id) != BM_OK) {
        BM_LOGE(TAG, "base event registration failed");
        return BM_ERR_INVALID;
    }

    for (uint8_t i = 0; i < EVENT_STORM_COUNT; i++) {
        bm_event_type_t type = (bm_event_type_t)(EVENT_STORM_BASE + i);
        if (bm_event_register_type(type, "STORM") != BM_OK ||
            bm_event_subscribe(type, storm_on_event, NULL,
                               &subscriber_id) != BM_OK) {
            BM_LOGE(TAG, "storm event %u registration failed", (unsigned)i);
            return BM_ERR_INVALID;
        }
    }

    return BM_OK;
}

static int publish_sensor_sample(void) {
    sensor_data_t *sample = bm_mempool_alloc(&sensor_pool);
    if (!sample) {
        BM_LOGW(TAG, "mempool alloc failed at cycle %u", (unsigned)sensor_cycle);
        return BM_ERR_NO_MEM;
    }

    sample->temp = (uint16_t)(22U + sensor_cycle % 5U);
    sample->humidity = (uint16_t)(44U + sensor_cycle % 5U);
    sample->timestamp = sensor_cycle;
    sample->status = 0;

    int rc = bm_event_publish_copy(EVENT_TEMP, 1, sample, sizeof(*sample));
    bm_mempool_free(&sensor_pool, sample);
    return rc;
}

/** 发布一组不同优先级的事件以验证调度顺序 */
static int publish_storm(void) {
    static const bm_event_priority_t priorities[EVENT_STORM_COUNT] = {
        2, 0, 3, 1, 0
    };

    BM_LOGI(TAG, "publishing event storm");
    for (uint8_t i = 0; i < EVENT_STORM_COUNT; i++) {
        int rc = bm_event_publish_copy(
            (bm_event_type_t)(EVENT_STORM_BASE + i),
            priorities[i],
            NULL,
            0);
        if (rc != BM_OK) {
            return rc;
        }
    }
    return BM_OK;
}

/** 校验风暴事件是否按优先级 0,0,1,2,3 顺序处理 */
static int storm_order_is_valid(void) {
    static const uint8_t expected[EVENT_STORM_COUNT] = {0, 0, 1, 2, 3};

    if (storm_event_count != EVENT_STORM_COUNT) {
        return 0;
    }
    for (uint8_t i = 0; i < EVENT_STORM_COUNT; i++) {
        if (storm_priorities[i] != expected[i]) {
            return 0;
        }
    }
    return 1;
}

static int app_init(void) {
    bm_event_reset();
    if (register_events() != BM_OK ||
        bm_module_init_all() != BM_OK ||
        bm_module_start_all() != BM_OK) {
        BM_LOGE(TAG, "app init failed");
        return BM_ERR_NOT_INIT;
    }

    if (bm_wdg_register("sensor") != BM_OK ||
        bm_wdg_register("comm") != BM_OK ||
        bm_wdg_register("display") != BM_OK) {
        BM_LOGE(TAG, "watchdog register failed");
        return BM_ERR_NO_MEM;
    }

    example_print("[wdg] sensor, comm, display registered\n");
    BM_LOGI(TAG, "watchdog modules registered");
    return BM_OK;
}

int main(void) {
    uint8_t storm_published = 0;
    uint8_t pass_sent = 0;

    bm_hal_uart_init(NULL);
    example_print("Zeplod Example: full_system\n");
    BM_LOGI(TAG, "full_system example start");

    if (app_init() != BM_OK) {
        BM_LOGE(TAG, "app init failed");
        example_print("EXAMPLE_FULL: INIT FAILED\n");
        return 1;
    }

    while (1) {
        example_delay_cycles(1000000U);
        sensor_cycle++;

        if (publish_sensor_sample() != BM_OK) {
            example_print("[sensor] sample dropped\n");
        }

        if (sensor_cycle == 3U && !storm_published) {
            storm_published = (publish_storm() == BM_OK);
        }

        if (sensor_cycle == 5U && !fault_observed) {
            uint8_t code = 3;
            bm_event_publish_copy(EVENT_SENSOR_FAULT, 0, &code, sizeof(code));
        }

        bm_event_process(8);

        if (storm_published && storm_event_count == EVENT_STORM_COUNT) {
            example_print(storm_order_is_valid()
                ? "[storm] priority order verified\n"
                : "[storm] priority order failed\n");
            BM_LOGI(TAG, "storm priority order %s",
                    storm_order_is_valid() ? "verified" : "failed");
            storm_published = 0;
        }

        bm_wdg_feed_module("sensor");
        bm_wdg_feed_module("comm");
        bm_wdg_feed_module("display");
        bm_wdg_feed();

        if (storm_event_count == EVENT_STORM_COUNT &&
            storm_order_is_valid() &&
            fault_observed &&
            sensor_cycle >= 6U &&
            !pass_sent) {
            BM_LOGI(TAG, "example PASS at cycle %u", (unsigned)sensor_cycle);
            example_print("EXAMPLE_FULL: PASS\n");
            pass_sent = 1;
        }
    }
}
