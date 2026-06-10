/* Lightweight non-blocking serial CLI. */
#include "bm_shell.h"
#include "bm_hal_uart.h"
#include <string.h>

/* Simple strcmp (avoid full libc dependency if possible) */
static int _strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void _puts(const char *s) {
    bm_hal_uart_send((const uint8_t *)s, strlen(s));
}

void bm_shell_init(bm_shell_t *shell) {
    if (!shell) return;
    memset(shell->buf, 0, sizeof(shell->buf));
    shell->write_idx = 0;
    shell->read_idx = 0;
    shell->line_len = 0;
    shell->cursor = 0;
    shell->cmd_count = 0;
}

int bm_shell_register(bm_shell_t *shell, const char *name,
                      bm_shell_cmd_fn_t fn, const char *help) {
    if (!shell || !name || !fn) return BM_ERR_INVALID;
    if (shell->cmd_count >= BM_CONFIG_SHELL_MAX_CMDS) return BM_ERR_NO_MEM;

    shell->cmds[shell->cmd_count].name = name;
    shell->cmds[shell->cmd_count].fn = fn;
    shell->cmds[shell->cmd_count].help = help;
    shell->cmd_count++;
    return BM_OK;
}

/* Tokenize line in-place; returns argc */
static int _tokenize(char *line, char *argv[], int max_argv) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max_argv) {
        /* skip leading spaces */
        while (*p == ' ' || *p == '\t') *p++ = '\0';
        if (!*p) break;
        argv[argc++] = p;
        /* skip to end of token */
        while (*p && *p != ' ' && *p != '\t') p++;
    }
    return argc;
}

int bm_shell_exec(bm_shell_t *shell, char *line) {
    if (!shell || !line) return BM_ERR_INVALID;

    char *argv[BM_CONFIG_SHELL_MAX_ARGS];
    int argc = _tokenize(line, argv, BM_CONFIG_SHELL_MAX_ARGS);
    if (argc == 0) return BM_OK;

    for (uint8_t i = 0; i < shell->cmd_count; i++) {
        if (_strcmp(argv[0], shell->cmds[i].name) == 0) {
            return shell->cmds[i].fn(argc, argv);
        }
    }

    _puts("unknown command: ");
    _puts(argv[0]);
    _puts("\r\n");
    return BM_ERR_NOT_FOUND;
}

void bm_shell_feed(bm_shell_t *shell, char c) {
    if (!shell) return;

    /* Handle backspace / delete */
    if (c == '\b' || c == 0x7F) {
        if (shell->cursor > 0) {
            shell->cursor--;
            _puts("\b \b");
        }
        return;
    }

    /* Echo printable characters */
    if (c >= 0x20 && c < 0x7F) {
        if (shell->cursor < BM_CONFIG_SHELL_BUF_SIZE - 1) {
            shell->buf[shell->cursor++] = c;
            bm_hal_uart_send((const uint8_t *)&c, 1);
        }
        return;
    }

    /* Execute on CR or LF */
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

void bm_shell_poll(bm_shell_t *shell) {
    if (!shell) return;
    uint8_t c;
    while (bm_hal_uart_recv(&c, 1) == 1) {
        bm_shell_feed(shell, (char)c);
    }
}

void bm_shell_puts(const char *s) {
    if (s) _puts(s);
}
