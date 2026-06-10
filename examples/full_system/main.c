#include "bm_core.h"
#include "bm_hal_uart.h"
#include "bm_module.h"
#include "bm_wdg.h"
#include "example_support.h"

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

static int fault_init(void) {
    example_print("[mod] fault init\n");
    return BM_OK;
}

static int fault_start(void) {
    return BM_OK;
}

static int comm_init(void) {
    example_print("[mod] comm init\n");
    return BM_OK;
}

static int comm_start(void) {
    return BM_OK;
}

static int comm_stop(void) {
    return BM_OK;
}

static int comm_deinit(void) {
    return BM_OK;
}

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

const bm_module_t _bm_module_table[] = {
    { .name = "fault", .priority = 0, .init = fault_init,
      .start = fault_start },
    { .name = "comm", .priority = 1, .init = comm_init,
      .start = comm_start, .stop = comm_stop, .deinit = comm_deinit },
    { .name = "sensor", .priority = 2, .init = sensor_init,
      .start = sensor_start, .stop = sensor_stop, .deinit = sensor_deinit },
    { .name = "display", .priority = 3, .init = display_init,
      .start = display_start },
};
const uint32_t _bm_module_count = 4;

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
    fault_observed = 1;
}

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
        return BM_ERR_INVALID;
    }

    for (uint8_t i = 0; i < EVENT_STORM_COUNT; i++) {
        bm_event_type_t type = (bm_event_type_t)(EVENT_STORM_BASE + i);
        if (bm_event_register_type(type, "STORM") != BM_OK ||
            bm_event_subscribe(type, storm_on_event, NULL,
                               &subscriber_id) != BM_OK) {
            return BM_ERR_INVALID;
        }
    }

    return BM_OK;
}

static int publish_sensor_sample(void) {
    sensor_data_t *sample = bm_mempool_alloc(&sensor_pool);
    if (!sample) {
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

static int publish_storm(void) {
    static const bm_event_priority_t priorities[EVENT_STORM_COUNT] = {
        2, 0, 3, 1, 0
    };

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
        return BM_ERR_NOT_INIT;
    }

    if (bm_wdg_register("sensor") != BM_OK ||
        bm_wdg_register("comm") != BM_OK ||
        bm_wdg_register("display") != BM_OK) {
        return BM_ERR_NO_MEM;
    }

    example_print("[wdg] sensor, comm, display registered\n");
    return BM_OK;
}

int main(void) {
    uint8_t storm_published = 0;
    uint8_t pass_sent = 0;

    bm_hal_uart_init(NULL);
    example_print("Zeplod Example: full_system\n");

    if (app_init() != BM_OK) {
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
            example_print("EXAMPLE_FULL: PASS\n");
            pass_sent = 1;
        }
    }
}
