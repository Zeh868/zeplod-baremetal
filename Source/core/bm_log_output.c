/**
 * @file bm_log_output.c
 * @brief Default log output provider.
 *
 * Kept in a separate translation unit so applications can provide a strong
 * bm_log_output symbol on toolchains without weak symbol support.
 */
#include "bm_log.h"

#if BM_CONFIG_LOG_USE_STDIO
#include <stdio.h>
#endif

void bm_log_output(const char *buf, size_t len) {
#if BM_CONFIG_LOG_USE_STDIO
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
#else
    (void)buf;
    (void)len;
#endif
}
