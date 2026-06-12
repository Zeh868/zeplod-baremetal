#include "bm_log.h"
#include "bm_module.h"
#include "example_support.h"

#define TAG "full_system"

static int display_init(void) {
    example_print("[mod] display init\n");
    BM_LOGI(TAG, "module display init");
    return BM_OK;
}

static int display_start(void) {
    return BM_OK;
}

BM_MODULE_DEFINE(display, 3, display_init, display_start, NULL, NULL);
