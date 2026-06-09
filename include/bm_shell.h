/* include/bm_shell.h — lightweight non-blocking serial CLI */
#ifndef BM_SHELL_H
#define BM_SHELL_H

#include "bm_core.h"

#ifndef BM_CONFIG_SHELL_BUF_SIZE
#define BM_CONFIG_SHELL_BUF_SIZE 64
#endif

#ifndef BM_CONFIG_SHELL_MAX_ARGS
#define BM_CONFIG_SHELL_MAX_ARGS 4
#endif

#ifndef BM_CONFIG_SHELL_MAX_CMDS
#define BM_CONFIG_SHELL_MAX_CMDS 8
#endif

typedef int (*bm_shell_cmd_fn_t)(int argc, char *argv[]);

typedef struct {
    const char       *name;
    bm_shell_cmd_fn_t fn;
    const char       *help;
} bm_shell_cmd_t;

typedef struct {
    char          buf[BM_CONFIG_SHELL_BUF_SIZE];
    uint8_t       write_idx;
    uint8_t       read_idx;
    uint8_t       line_len;
    uint8_t       cursor;
    bm_shell_cmd_t cmds[BM_CONFIG_SHELL_MAX_CMDS];
    uint8_t       cmd_count;
} bm_shell_t;

/* Static instance macro */
#define BM_SHELL_DEFINE(name) \
    static bm_shell_t name = { .write_idx = 0, .read_idx = 0, .line_len = 0, .cursor = 0, .cmd_count = 0 }

/* Initialize shell (clear buffers, no registered commands) */
void bm_shell_init(bm_shell_t *shell);

/* Register a command. Returns BM_OK or BM_ERR_NO_MEM. */
int bm_shell_register(bm_shell_t *shell, const char *name,
                      bm_shell_cmd_fn_t fn, const char *help);

/* Feed one incoming character (e.g. from UART ISR).
 * If a complete line is formed, it is executed immediately. */
void bm_shell_feed(bm_shell_t *shell, char c);

/* Poll UART for new characters and process. Non-blocking. */
void bm_shell_poll(bm_shell_t *shell);

/* Execute a command line directly. Modifies line in-place for tokenization. */
int bm_shell_exec(bm_shell_t *shell, char *line);

/* Print a string via UART HAL */
void bm_shell_puts(const char *s);

#endif /* BM_SHELL_H */
