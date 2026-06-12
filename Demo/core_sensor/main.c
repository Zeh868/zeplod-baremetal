/**
 * @file main.c
 * @brief 核心域传感器示例：模块生命周期 + 事件发布/订阅
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
#include "example_support.h"

#define TAG "core_sensor"

/** 温度采样事件类型 */
#define EVENT_TEMP 1

/** 传感器采样数据结构 */
typedef struct {
    uint16_t temp;
    uint16_t humidity;
    uint32_t timestamp;
    uint8_t status;
} sensor_data_t;

BM_MEMPOOL_DEFINE(sensor_pool, sensor_data_t, 4);

/** 打印一次温湿度读数 */
static void print_measurement(const char *prefix, const sensor_data_t *data) {
    example_print(prefix);
    example_print_u32(data->temp);
    example_print(", humidity=");
    example_print_u32(data->humidity);
    example_print("\n");
}

static void logger_on_temp(const bm_event_t *event, void *user_data) {
    (void)user_data;
    print_measurement("[log] temperature=", event->data);
}

static void display_on_temp(const bm_event_t *event, void *user_data) {
    (void)user_data;
    print_measurement("[display] temperature=", event->data);
}

/** 从内存池分配样本并发布温度事件 */
static int publish_sensor_sample(uint32_t cycle) {
    sensor_data_t *sample = bm_mempool_alloc(&sensor_pool);
    if (!sample) {
        BM_LOGW(TAG, "mempool alloc failed at cycle %u", (unsigned)cycle);
        return BM_ERR_NO_MEM;
    }

    sample->temp = (uint16_t)(23U + cycle / 2U);
    sample->humidity = (uint16_t)(45U + cycle / 2U);
    sample->timestamp = cycle;
    sample->status = 0;

    int rc = bm_event_publish_copy(EVENT_TEMP, 1, sample, sizeof(*sample));
    bm_mempool_free(&sensor_pool, sample);
    return rc;
}

/** 注册事件、订阅者并启动所有模块 */
static int app_init(void) {
    bm_event_subscriber_id_t display_id;
    bm_event_subscriber_id_t logger_id;

    bm_event_reset();
    if (bm_event_register_type(EVENT_TEMP, "TEMP") != BM_OK ||
        bm_event_subscribe(EVENT_TEMP, display_on_temp, NULL, &display_id) != BM_OK ||
        bm_event_subscribe(EVENT_TEMP, logger_on_temp, NULL, &logger_id) != BM_OK) {
        BM_LOGE(TAG, "event setup failed");
        return BM_ERR_INVALID;
    }

    if (bm_module_init_all() != BM_OK || bm_module_start_all() != BM_OK) {
        BM_LOGE(TAG, "module init/start failed");
        return BM_ERR_NOT_INIT;
    }

    BM_LOGI(TAG, "app init ok");
    return BM_OK;
}

int main(void) {
    bm_hal_uart_init(NULL);
    example_print("Zeplod Example: core_sensor\n");
    BM_LOGI(TAG, "core_sensor example start");

    if (app_init() != BM_OK) {
        BM_LOGE(TAG, "app init failed");
        example_print("EXAMPLE_CORE: INIT FAILED\n");
        return 1;
    }

    uint32_t cycle = 0;
    uint8_t pass_sent = 0;

    while (1) {
        example_delay_cycles(1000000U);
        cycle++;

        if ((cycle % 2U) == 0U && publish_sensor_sample(cycle) != BM_OK) {
            example_print("[sensor] sample dropped\n");
        }

        bm_event_process(4);

        if (cycle >= 4U && !pass_sent) {
            BM_LOGI(TAG, "example PASS at cycle %u", (unsigned)cycle);
            example_print("EXAMPLE_CORE: PASS\n");
            pass_sent = 1;
        }
    }
}
