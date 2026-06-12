#include "bm_log.h"
#include "bm_module.h"
#include "example_support.h"

#define TAG "full_system"

static int sensor_init(void) {
    example_print("[mod] sensor init\n");
    BM_LOGI(TAG, "module sensor init");
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

BM_MODULE_DEFINE_EX(sensor, 3, BM_MODULE_FLAG_WDG,
    sensor_init, sensor_start, sensor_stop, sensor_deinit);
