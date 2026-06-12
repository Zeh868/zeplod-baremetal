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

/**
 * @brief 按等级格式化并输出一条日志
 *
 * @param level 日志等级
 * @param tag 模块标签
 * @param fmt printf 风格格式字符串
 */
void bm_log(bm_log_level_t level, const char *tag, const char *fmt, ...) {
    char buf[BM_CONFIG_LOG_BUF_SIZE];
    int level_index = (int)level;
    int prefix_len;
    va_list ap;

    if (!fmt) {
        return;
    }
    if (level_index < (int)BM_LOG_ERROR) {
        level_index = (int)BM_LOG_ERROR;
    }
    if (level_index > (int)BM_LOG_TRACE) {
        level_index = (int)BM_LOG_TRACE;
    }
    if (!tag) {
        tag = "bm";
    }

    prefix_len = snprintf(buf, sizeof(buf), "[%s][%s] ",
                          k_level_chars[level_index], tag);
    if (prefix_len < 0 || (size_t)prefix_len >= sizeof(buf)) {
        return;
    }

    va_start(ap, fmt);
    {
        int body_len = vsnprintf(buf + prefix_len,
                                 sizeof(buf) - (size_t)prefix_len, fmt, ap);

        va_end(ap);
        if (body_len < 0 ||
            (size_t)body_len >= sizeof(buf) - (size_t)prefix_len) {
            return;
        }
    }

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
