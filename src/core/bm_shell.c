/**
 * @file bm_shell.c
 * @brief 轻量级非阻塞串口命令行实现
 *
 * 字符回显、退格处理与命令分词执行；通过 UART HAL 收发。
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
#include "bm_shell.h"
#include "bm_hal_uart.h"
#include "bm_log.h"

#include <string.h>

/**
 * @brief 简易字符串比较（减少对完整 libc 的依赖）
 *
 * @param a 第一个字符串
 * @param b 第二个字符串
 * @return 相等返回 0；不等返回字符差值
 */
static int _strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

/**
 * @brief 通过 UART HAL 发送字符串
 *
 * @param s 以 NUL 结尾的字符串
 */
static void _puts(const char *s) {
    bm_hal_uart_send((const uint8_t *)s, strlen(s));
}

/**
 * @brief 初始化 Shell 上下文与命令表
 *
 * @param shell Shell 实例指针
 */
void bm_shell_init(bm_shell_t *shell) {
    if (!shell) return;
    memset(shell->buf, 0, sizeof(shell->buf));
    shell->write_idx = 0;
    shell->read_idx = 0;
    shell->line_len = 0;
    shell->cursor = 0;
    shell->cmd_count = 0;
    BM_LOGI("shell", "init");
}

/**
 * @brief 注册一条 Shell 命令
 *
 * @param shell Shell 实例指针
 * @param name 命令名称
 * @param fn 命令处理函数
 * @param help 帮助文本（可为 NULL）
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_NO_MEM 命令表已满
 */
int bm_shell_register(bm_shell_t *shell, const char *name,
                      bm_shell_cmd_fn_t fn, const char *help) {
    if (!shell || !name || !fn) return BM_ERR_INVALID;
    if (shell->cmd_count >= BM_CONFIG_SHELL_MAX_CMDS) {
        BM_LOGW("shell", "register table full");
        return BM_ERR_NO_MEM;
    }

    shell->cmds[shell->cmd_count].name = name;
    shell->cmds[shell->cmd_count].fn = fn;
    shell->cmds[shell->cmd_count].help = help;
    shell->cmd_count++;
    BM_LOGD("shell", "cmd '%s' registered", name);
    return BM_OK;
}

/**
 * @brief 就地分词命令行，返回参数个数
 *
 * @param line 可修改的命令行缓冲区
 * @param argv 参数指针数组
 * @param max_argv 最大参数个数
 * @return 解析得到的 argc
 */
static int _tokenize(char *line, char *argv[], int max_argv) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max_argv) {
        while (*p == ' ' || *p == '\t') *p++ = '\0';
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
    }
    return argc;
}

/**
 * @brief 解析并执行一条命令行
 *
 * @param shell Shell 实例指针
 * @param line 可修改的命令行缓冲区
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_NOT_FOUND 未知命令；其他为命令返回值
 */
int bm_shell_exec(bm_shell_t *shell, char *line) {
    if (!shell || !line) return BM_ERR_INVALID;

    char *argv[BM_CONFIG_SHELL_MAX_ARGS];
    int argc = _tokenize(line, argv, BM_CONFIG_SHELL_MAX_ARGS);
    if (argc == 0) {
        return BM_OK;
    }
    if (argc >= BM_CONFIG_SHELL_MAX_ARGS) {
        BM_LOGW("shell", "too many arguments");
        return BM_ERR_INVALID;
    }
    if (shell->cmd_count > BM_CONFIG_SHELL_MAX_CMDS) {
        return BM_ERR_INVALID;
    }

    for (uint8_t i = 0; i < shell->cmd_count; i++) {
        if (_strcmp(argv[0], shell->cmds[i].name) == 0) {
            BM_LOGD("shell", "exec '%s'", argv[0]);
            return shell->cmds[i].fn(argc, argv);
        }
    }

    BM_LOGW("shell", "unknown cmd '%s'", argv[0]);
    _puts("unknown command: ");
    _puts(argv[0]);
    _puts("\r\n");
    return BM_ERR_NOT_FOUND;
}

/**
 * @brief 处理单个输入字符（回显、退格、回车执行）
 *
 * @param shell Shell 实例指针
 * @param c 输入字符
 */
void bm_shell_feed(bm_shell_t *shell, char c) {
    if (!shell) return;

    if (c == '\b' || c == 0x7F) {
        if (shell->cursor > 0) {
            shell->cursor--;
            shell->buf[shell->cursor] = '\0';
            _puts("\b \b");
        }
        return;
    }

    if (c >= 0x20 && c < 0x7F) {
        if (shell->cursor < BM_CONFIG_SHELL_BUF_SIZE - 1) {
            shell->buf[shell->cursor++] = c;
            bm_hal_uart_send((const uint8_t *)&c, 1);
        }
        return;
    }

    if (c == '\r' || c == '\n') {
        _puts("\r\n");
        if (shell->cursor > 0) {
            shell->buf[shell->cursor] = '\0';
            bm_shell_exec(shell, shell->buf);
            shell->cursor = 0;
        }
        _puts("$ ");
    }
}

/**
 * @brief 轮询 UART 接收缓冲区并逐字符喂入 Shell
 *
 * @param shell Shell 实例指针
 */
void bm_shell_poll(bm_shell_t *shell) {
    if (!shell) return;
    uint8_t c;
    while (bm_hal_uart_recv(&c, 1) == 1) {
        bm_shell_feed(shell, (char)c);
    }
}

/**
 * @brief 通过 Shell 输出通道打印字符串
 *
 * @param s 以 NUL 结尾的字符串
 */
void bm_shell_puts(const char *s) {
    if (s) _puts(s);
}
