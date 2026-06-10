/**
 * @file bm_log.c
 * @brief 分级日志实现：格式化消息并调用输出钩子
 *
 * 默认 bm_log_output 为空操作；启用 BM_CONFIG_LOG_USE_STDIO 时写入 stdout。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#include "bm_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/** 等级前缀字符 */
static const char *const k_level_chars[] = {
    "E", "W", "I", "D", "T"
};

#if !BM_CONFIG_LOG_USE_STDIO
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak))
#endif
/**
 * @brief 日志输出钩子（弱符号默认空实现）
 *
 * @param buf 格式化后的日志缓冲区
 * @param len 缓冲区有效字节长度
 */
void bm_log_output(const char *buf, size_t len) {
    (void)buf;
    (void)len;
}
#else
/**
 * @brief 日志输出钩子（stdio 实现，写入 stdout）
 *
 * @param buf 格式化后的日志缓冲区
 * @param len 缓冲区有效字节长度
 */
void bm_log_output(const char *buf, size_t len) {
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
}
#endif

/**
 * @brief 按等级格式化并输出一条日志
 *
 * @param level 日志等级
 * @param tag 模块标签
 * @param fmt printf 风格格式字符串
 */
void bm_log(bm_log_level_t level, const char *tag, const char *fmt, ...) {
    char buf[BM_CONFIG_LOG_BUF_SIZE];
    int prefix_len;
    va_list ap;

    if (level > BM_LOG_TRACE) {
        level = BM_LOG_TRACE;
    }
    if (!tag) {
        tag = "bm";
    }

    prefix_len = snprintf(buf, sizeof(buf), "[%s][%s] ",
                          k_level_chars[level], tag);
    if (prefix_len < 0 || (size_t)prefix_len >= sizeof(buf)) {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(buf + prefix_len, sizeof(buf) - (size_t)prefix_len, fmt, ap);
    va_end(ap);

    /* 保证单行换行结尾 */
    {
        size_t len = strlen(buf);
        if (len + 1u < sizeof(buf) && (len == 0u || buf[len - 1u] != '\n')) {
            buf[len] = '\n';
            buf[len + 1u] = '\0';
            len++;
        }
        bm_log_output(buf, len);
    }
}
