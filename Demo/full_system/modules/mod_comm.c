#include "bm_log.h"
#include "bm_module.h"
#include "example_support.h"

#define TAG "full_system"

static int comm_init(void) {
    example_print("[mod] comm init\n");
    BM_LOGI(TAG, "module comm init");
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

BM_MODULE_DEFINE(comm, 1, comm_init, comm_start, comm_stop, comm_deinit);
