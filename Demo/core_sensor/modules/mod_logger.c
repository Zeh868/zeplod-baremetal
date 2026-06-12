#include "bm_log.h"
#include "bm_module.h"
#include "example_support.h"

#define TAG "core_sensor"

static int logger_init(void) {
    example_print("[mod] logger init\n");
    BM_LOGI(TAG, "module logger init");
    return BM_OK;
}

static int logger_start(void) {
    return BM_OK;
}

BM_MODULE_DEFINE(logger, 1, logger_init, logger_start, NULL, NULL);
