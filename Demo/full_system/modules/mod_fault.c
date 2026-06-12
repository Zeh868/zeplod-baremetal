#include "bm_log.h"
#include "bm_module.h"
#include "example_support.h"

#define TAG "full_system"

static int fault_init(void) {
    example_print("[mod] fault init\n");
    BM_LOGI(TAG, "module fault init");
    return BM_OK;
}

static int fault_start(void) {
    return BM_OK;
}

BM_MODULE_DEFINE(fault, 0, fault_init, fault_start, NULL, NULL);
