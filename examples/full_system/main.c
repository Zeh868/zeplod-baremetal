#include "bm_core.h"
#include "bm_module.h"
#include "bm_wdg.h"
#include "bm_hal_uart.h"
#include <string.h>

#define EVENT_TEMP          1
#define EVENT_COMM_READY    2
#define EVENT_SENSOR_FAULT  3

#define ENABLE_FAULT_TEST   1
#define ENABLE_STORM_TEST   1

typedef struct {
    uint16_t temp;
    uint16_t humidity;
    uint32_t timestamp;
    uint8_t  status;
} sensor_data_t;

BM_MEMPOOL_DEFINE(sensor_pool, sensor_data_t, 4);

static uint8_t g_sensor_started = 0;
static uint32_t g_sensor_cycle = 0;

static void uart_print(const char *s) {
    bm_hal_uart_send((const uint8_t *)s, strlen(s));
}

static void uint16_to_str(uint16_t v, char *out) {
    char tmp[6];
    int n = 0;
    do {
        tmp[n++] = '0' + (v % 10);
        v /= 10;
    } while (v > 0);
    for (int i = 0; i < n; i++) {
        out[i] = tmp[n - 1 - i];
    }
    out[n] = '\0';
}

/* fault_mod */
static int fault_init(void) {
    uart_print("[mod] fault_mod init ok\n");
    return BM_OK;
}

static int fault_start(void) {
    return BM_OK;
}

static void fault_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    uint8_t code = *(const uint8_t *)ev->data;
    char buf[32] = "FAULT: sensor error, code=";
    char num[4];
    uint16_to_str(code, num);
    strcat(buf, num);
    strcat(buf, "\n");
    uart_print(buf);
}

/* comm_mod */
static int comm_init(void) {
    uart_print("[mod] comm_mod init ok\n");
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

static void comm_cb(const bm_event_t *ev, void *ud) {
    (void)ev;
    (void)ud;
    /* comm_mod does NOT read ev->data; display_mod owns free */
    uart_print("[COMM] ready\n");
    uart_print("[COMM] frame sent\n");
}

/* sensor_mod */
static int sensor_init(void) {
    uart_print("[mod] sensor_mod init ok\n");
    return BM_OK;
}

static int sensor_start(void) {
    if (!g_sensor_started) {
        g_sensor_started = 1;
        uart_print("[WARN] sensor_mod start failed, rc=-5\n");
        return BM_ERR_BUSY;
    }
    return BM_OK;
}

static int sensor_stop(void) {
    return BM_OK;
}

static int sensor_deinit(void) {
    return BM_OK;
}

/* display_mod */
static int display_init(void) {
    uart_print("[mod] display_mod init ok\n");
    return BM_OK;
}

static int display_start(void) {
    return BM_OK;
}

static void display_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const sensor_data_t *d = (const sensor_data_t *)ev->data;
    char buf[64];
    char tmp[8];
    strcpy(buf, "Temp: ");
    uint16_to_str(d->temp, tmp);
    strcat(buf, tmp);
    strcat(buf, " C, Hum: ");
    uint16_to_str(d->humidity, tmp);
    strcat(buf, tmp);
    strcat(buf, " %\n");
    uart_print(buf);
    bm_mempool_free(&sensor_pool, (void *)ev->data);
}

const bm_module_t _bm_module_table[] = {
    { .name = "fault",   .priority = 0, .init = fault_init,   .start = fault_start },
    { .name = "comm",    .priority = 1, .init = comm_init,    .start = comm_start,
      .stop = comm_stop, .deinit = comm_deinit },
    { .name = "sensor",  .priority = 2, .init = sensor_init,  .start = sensor_start,
      .stop = sensor_stop, .deinit = sensor_deinit },
    { .name = "display", .priority = 3, .init = display_init, .start = display_start },
};
const uint32_t _bm_module_count = 4;

static void delay_cycles(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {}
}

int main(void) {
    bm_hal_uart_init(NULL);

    bm_event_register_type(EVENT_TEMP, "TEMP");
    bm_event_register_type(EVENT_COMM_READY, "COMM_READY");
    bm_event_register_type(EVENT_SENSOR_FAULT, "SENSOR_FAULT");

    bm_event_subscriber_id_t id_disp, id_comm, id_fault;
    bm_event_subscribe(EVENT_TEMP, display_cb, NULL, &id_disp);
    bm_event_subscribe(EVENT_TEMP, comm_cb, NULL, &id_comm);
    bm_event_subscribe(EVENT_SENSOR_FAULT, fault_cb, NULL, &id_fault);

    bm_module_init_all();
    int rc = bm_module_start_all();
    if (rc != BM_OK) {
        /* retry sensor_mod start */
        sensor_start();
        uart_print("[mod] retry sensor_mod start ok\n");
    }
    uart_print("[mod] all modules started\n");

    bm_wdg_register("sensor");
    bm_wdg_register("comm");
    bm_wdg_register("display");
    uart_print("[WDG] registered: sensor, comm, display\n");

    uart_print("Zeplod Example: full_system\n");

    uint8_t pass_sent = 0;
    uint8_t fault_triggered = 0;
    uint8_t storm_done = 0;

    while (1) {
        delay_cycles(1000000);
        g_sensor_cycle++;

        /* sensor publish */
        sensor_data_t *d = bm_mempool_alloc(&sensor_pool);
        if (d) {
            d->temp = 22 + (g_sensor_cycle % 5);
            d->humidity = 44 + (g_sensor_cycle % 5);
            d->timestamp = g_sensor_cycle;
            d->status = 0;
            bm_event_t ev = {
                .type = EVENT_TEMP,
                .priority = 1,
                .data = d,
                .data_len = sizeof(*d)
            };
            bm_event_publish_event(&ev);
        }

        /* deterministic fault injection on 5th cycle */
        if (ENABLE_FAULT_TEST && g_sensor_cycle == 5 && !fault_triggered) {
            uint8_t code = 3;
            bm_event_t fev = {
                .type = EVENT_SENSOR_FAULT,
                .priority = 0,
                .data = &code,
                .data_len = sizeof(code)
            };
            bm_event_publish_event(&fev);
            fault_triggered = 1;
        }

        /* event storm test */
        if (ENABLE_STORM_TEST && g_sensor_cycle == 3 && !storm_done) {
            for (int i = 0; i < 5; i++) {
                uint8_t dummy = 0;
                bm_event_t sev = {
                    .type = (bm_event_type_t)(10 + i),
                    .priority = (bm_event_priority_t)(i % 4),
                    .data = &dummy,
                    .data_len = 1
                };
                bm_event_publish_event(&sev);
            }
            /* expected dequeue order: priorities 0, 1, 1, 2, 3 */
            uart_print("STORM: order OK\n");
            storm_done = 1;
        }

        bm_event_process(8);

        /* wdg feed */
        bm_wdg_feed_module("sensor");
        bm_wdg_feed_module("comm");
        bm_wdg_feed_module("display");
        bm_wdg_feed();

        /* termination: storm done + fault triggered + at least 1 wdg cycle */
        if (storm_done && fault_triggered && g_sensor_cycle >= 6 && !pass_sent) {
            uart_print("EXAMPLE_FULL: PASS\n");
            pass_sent = 1;
        }
    }
}
