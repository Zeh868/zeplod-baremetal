#include "bm_core.h"
#include "bm_hal_uart.h"
#include "bm_module.h"
#include "example_support.h"

#define EVENT_TEMP 1

typedef struct {
    uint16_t temp;
    uint16_t humidity;
    uint32_t timestamp;
    uint8_t status;
} sensor_data_t;

BM_MEMPOOL_DEFINE(sensor_pool, sensor_data_t, 4);

static int sensor_init(void) {
    example_print("[mod] sensor init\n");
    return BM_OK;
}

static int sensor_start(void) {
    return BM_OK;
}

static int sensor_stop(void) {
    return BM_OK;
}

static int sensor_deinit(void) {
    return BM_OK;
}

static int display_init(void) {
    example_print("[mod] display init\n");
    return BM_OK;
}

static int display_start(void) {
    return BM_OK;
}

static int logger_init(void) {
    example_print("[mod] logger init\n");
    return BM_OK;
}

static int logger_start(void) {
    return BM_OK;
}

const bm_module_t _bm_module_table[] = {
    { .name = "display", .priority = 0, .init = display_init,
      .start = display_start },
    { .name = "logger", .priority = 1, .init = logger_init,
      .start = logger_start },
    { .name = "sensor", .priority = 2, .init = sensor_init,
      .start = sensor_start, .stop = sensor_stop, .deinit = sensor_deinit },
};
const uint32_t _bm_module_count = 3;

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

static int publish_sensor_sample(uint32_t cycle) {
    sensor_data_t *sample = bm_mempool_alloc(&sensor_pool);
    if (!sample) {
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

static int app_init(void) {
    bm_event_subscriber_id_t display_id;
    bm_event_subscriber_id_t logger_id;

    bm_event_reset();
    if (bm_event_register_type(EVENT_TEMP, "TEMP") != BM_OK ||
        bm_event_subscribe(EVENT_TEMP, display_on_temp, NULL, &display_id) != BM_OK ||
        bm_event_subscribe(EVENT_TEMP, logger_on_temp, NULL, &logger_id) != BM_OK) {
        return BM_ERR_INVALID;
    }

    if (bm_module_init_all() != BM_OK || bm_module_start_all() != BM_OK) {
        return BM_ERR_NOT_INIT;
    }

    return BM_OK;
}

int main(void) {
    bm_hal_uart_init(NULL);
    example_print("Zeplod Example: core_sensor\n");

    if (app_init() != BM_OK) {
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
            example_print("EXAMPLE_CORE: PASS\n");
            pass_sent = 1;
        }
    }
}
