// Diagnostic console: text commands and logs on the UART (J9) and USB CDC.
#ifndef CONSOLE_H
#define CONSOLE_H

void console_init(void);
void console_task(void);

void console_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Log line with a millisecond timestamp.
void console_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#define LOG(...) console_log(__VA_ARGS__)

// Print the retained log lines (oldest first).
void console_dump_history(void);

// Implemented in commands.c: run one command line (modified in place).
void commands_exec(char *line);

#endif
