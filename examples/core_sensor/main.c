#include "bm_core.h"
#include "bm_module.h"
#include "bm_hal_uart.h"
#include <string.h>

#define EVENT_TEMP  1

typedef struct {
    uint16_t temp;
    uint16_t humidity;
    uint32_t timestamp;
    uint8_t  status;
} sensor_data_t;

BM_MEMPOOL_DEFINE(sensor_pool, sensor_data_t, 4);

/* CMSIS SystemInit stub required by startup assembly */
void SystemInit(void) {}

static int sensor_init(void) {
    const char *msg = "[mod] sensor_mod init ok\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
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
    const char *msg = "[mod] display_mod init ok\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
    return BM_OK;
}

static int display_start(void) {
    return BM_OK;
}

static int logger_init(void) {
    const char *msg = "[mod] logger_mod init ok\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
    return BM_OK;
}

static int logger_start(void) {
    return BM_OK;
}

const bm_module_t _bm_module_table[] = {
    { .name = "display", .priority = 0, .init = display_init, .start = display_start },
    { .name = "logger",  .priority = 1, .init = logger_init,  .start = logger_start },
    { .name = "sensor",  .priority = 2, .init = sensor_init,  .start = sensor_start,
      .stop = sensor_stop, .deinit = sensor_deinit },
};
const uint32_t _bm_module_count = 3;

static void logger_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const sensor_data_t *d = (const sensor_data_t *)ev->data;
    char buf[48];
    char *p = buf;
    strcpy(p, "[LOG] temp=");
    p += strlen(p);
    /* simple uint16 to ascii */
    uint16_t v = d->temp;
    char tmp[6];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, ", hum=");
    p += strlen(p);
    v = d->humidity;
    n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, "\n");
    bm_hal_uart_send((const uint8_t *)buf, strlen(buf));
}

static void display_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const sensor_data_t *d = (const sensor_data_t *)ev->data;
    char buf[64];
    char *p = buf;
    strcpy(p, "Temp: ");
    p += strlen(p);
    uint16_t v = d->temp;
    char tmp[6];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, " C, Hum: ");
    p += strlen(p);
    v = d->humidity;
    n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, " %\n");
    bm_hal_uart_send((const uint8_t *)buf, strlen(buf));
    bm_mempool_free(&sensor_pool, (void *)ev->data);
}

static void delay_cycles(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {}
}

int main(void) {
    bm_hal_uart_init(NULL);
    bm_event_register_type(EVENT_TEMP, "TEMP");

    bm_event_subscriber_id_t id_disp, id_log;
    bm_event_subscribe(EVENT_TEMP, display_cb, NULL, &id_disp);
    bm_event_subscribe(EVENT_TEMP, logger_cb, NULL, &id_log);

    bm_module_init_all();
    bm_module_start_all();

    const char *msg = "[mod] all modules started\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));

    const char *header = "Zeplod Example: core_sensor\n";
    bm_hal_uart_send((const uint8_t *)header, strlen(header));

    uint32_t cycle = 0;
    uint8_t pass_sent = 0;

    while (1) {
        delay_cycles(1000000);
        cycle++;

        if (cycle % 2 == 0) {
            sensor_data_t *d = bm_mempool_alloc(&sensor_pool);
            if (d) {
                d->temp = 23 + (cycle / 2);
                d->humidity = 45 + (cycle / 2);
                d->timestamp = cycle;
                d->status = 0;
                bm_event_t ev = {
                    .type = EVENT_TEMP,
                    .priority = 1,
                    .data = d,
                    .data_len = sizeof(*d)
                };
                bm_event_publish_event(&ev);
            }
        }

        bm_event_process(4);

        if (cycle >= 4 && !pass_sent) {
            const char *pass = "EXAMPLE_CORE: PASS\n";
            bm_hal_uart_send((const uint8_t *)pass, strlen(pass));
            pass_sent = 1;
        }
    }
}
