/**
 * @file bm_shell.h
 * @brief 轻量级非阻塞串口命令行
 *
 * 支持字符流喂入、命令注册与 UART 轮询，适用于裸机调试。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 */
#ifndef BM_SHELL_H
#define BM_SHELL_H

#include "bm_types.h"

#include <stdint.h>

#ifndef BM_CONFIG_SHELL_BUF_SIZE
#define BM_CONFIG_SHELL_BUF_SIZE 64
#endif

#ifndef BM_CONFIG_SHELL_MAX_ARGS
#define BM_CONFIG_SHELL_MAX_ARGS 4
#endif

#ifndef BM_CONFIG_SHELL_MAX_CMDS
#define BM_CONFIG_SHELL_MAX_CMDS 8
#endif

/** 命令处理函数 */
typedef int (*bm_shell_cmd_fn_t)(int argc, char *argv[]);

/** 命令注册条目 */
typedef struct {
    const char       *name;
    bm_shell_cmd_fn_t fn;
    const char       *help;
} bm_shell_cmd_t;

/** Shell 实例状态 */
typedef struct {
    char          buf[BM_CONFIG_SHELL_BUF_SIZE];
    uint8_t       write_idx;
    uint8_t       read_idx;
    uint8_t       line_len;
    uint8_t       cursor;
    bm_shell_cmd_t cmds[BM_CONFIG_SHELL_MAX_CMDS];
    uint8_t       cmd_count;
} bm_shell_t;

/** 静态定义 Shell 实例 */
#define BM_SHELL_DEFINE(name) \
    static bm_shell_t name = { .write_idx = 0, .read_idx = 0, .line_len = 0, .cursor = 0, .cmd_count = 0 }

/**
 * @brief 初始化 Shell 实例
 *
 * @param shell Shell 实例指针
 */
void bm_shell_init(bm_shell_t *shell);

/**
 * @brief 注册 Shell 命令
 *
 * @param shell Shell 实例指针
 * @param name 命令名称
 * @param fn 命令处理函数
 * @param help 帮助文本（可为 NULL）
 * @return BM_OK 成功；BM_ERR_NO_MEM 命令表已满；BM_ERR_INVALID 参数无效
 */
int bm_shell_register(bm_shell_t *shell, const char *name,
                      bm_shell_cmd_fn_t fn, const char *help);

/**
 * @brief 喂入单个字符，完整行到达时立即执行
 *
 * 非 ISR 安全；仅从主循环或 bm_shell_poll 调用。
 *
 * @param shell Shell 实例指针
 * @param c 输入字符
 */
void bm_shell_feed(bm_shell_t *shell, char c);

/**
 * @brief 轮询 UART 并处理新字符（非阻塞）
 *
 * @param shell Shell 实例指针
 */
void bm_shell_poll(bm_shell_t *shell);

/**
 * @brief 直接执行命令行（就地分词，会修改 line 内容）
 *
 * @param shell Shell 实例指针
 * @param line 命令行缓冲区（可被修改）
 * @return 命令处理函数的返回值；未找到命令时返回 BM_ERR_NOT_FOUND
 */
int bm_shell_exec(bm_shell_t *shell, char *line);

/**
 * @brief 通过 UART 输出字符串
 *
 * @param s 以 NUL 结尾的字符串
 */
void bm_shell_puts(const char *s);

#endif /* BM_SHELL_H */
