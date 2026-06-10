/**
 * @file bm_log.h
 * @brief 分级日志接口（ERROR / WARN / INFO / DEBUG / TRACE）
 *
 * 通过 BM_CONFIG_ENABLE_LOG 与 BM_CONFIG_LOG_LEVEL 在编译期裁剪。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 */
#ifndef BM_LOG_H
#define BM_LOG_H

#include <stddef.h>

typedef enum {
    BM_LOG_ERROR = 0,
    BM_LOG_WARN  = 1,
    BM_LOG_INFO  = 2,
    BM_LOG_DEBUG = 3,
    BM_LOG_TRACE = 4
} bm_log_level_t;

#ifndef BM_CONFIG_ENABLE_LOG
#define BM_CONFIG_ENABLE_LOG 1
#endif

#ifndef BM_CONFIG_LOG_LEVEL
#define BM_CONFIG_LOG_LEVEL BM_LOG_INFO
#endif

#ifndef BM_CONFIG_LOG_BUF_SIZE
#define BM_CONFIG_LOG_BUF_SIZE 128
#endif

#ifndef BM_CONFIG_LOG_USE_STDIO
#define BM_CONFIG_LOG_USE_STDIO 0
#endif

void bm_log(bm_log_level_t level, const char *tag, const char *fmt, ...);
void bm_log_output(const char *buf, size_t len);

#if BM_CONFIG_ENABLE_LOG

#define BM_LOG(level, tag, ...) do { \
        if ((level) <= BM_CONFIG_LOG_LEVEL) { \
            bm_log((level), (tag), __VA_ARGS__); \
        } \
    } while (0)

#define BM_LOGE(tag, ...) BM_LOG(BM_LOG_ERROR, tag, __VA_ARGS__)
#define BM_LOGW(tag, ...) BM_LOG(BM_LOG_WARN,  tag, __VA_ARGS__)
#define BM_LOGI(tag, ...) BM_LOG(BM_LOG_INFO,  tag, __VA_ARGS__)
#define BM_LOGD(tag, ...) BM_LOG(BM_LOG_DEBUG, tag, __VA_ARGS__)
#define BM_LOGT(tag, ...) BM_LOG(BM_LOG_TRACE, tag, __VA_ARGS__)

#else

#define BM_LOG(level, tag, ...)   ((void)0)
#define BM_LOGE(tag, ...)         ((void)0)
#define BM_LOGW(tag, ...)         ((void)0)
#define BM_LOGI(tag, ...)         ((void)0)
#define BM_LOGD(tag, ...)         ((void)0)
#define BM_LOGT(tag, ...)         ((void)0)

#endif /* BM_CONFIG_ENABLE_LOG */

#endif /* BM_LOG_H */
