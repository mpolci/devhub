#include "console/console.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "board/board.h"
#include "drivers/uart.h"
#include "usb/usb_dev.h"

#define LINE_MAX 80

typedef struct {
    char buf[LINE_MAX];
    uint8_t len;
    int (*getc)(void);
} line_t;

static line_t lines[2];

static void out(const char *s, uint16_t len) {
    uart_write(s, len);
    usb_cdc_write(s, len);
}

static void vprint(const char *fmt, va_list ap) {
    char buf[160];
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    if (n < 0) {
        return;
    }
    if (n >= (int)sizeof buf) {
        n = sizeof buf - 1;
    }
    out(buf, (uint16_t)n);
}

void console_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
}

void console_log(const char *fmt, ...) {
    uint32_t t = board_millis();
    console_printf("[%lu.%03lu] ", (unsigned long)(t / 1000), (unsigned long)(t % 1000));
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
    out("\r\n", 2);
}

void console_init(void) {
    uart_init();
    lines[0].getc = uart_getc;
    lines[1].getc = usb_cdc_getc;
}

static void feed(line_t *l, char c) {
    if (c == '\r' || c == '\n') {
        if (l->len) {
            out("\r\n", 2);
            l->buf[l->len] = 0;
            l->len = 0;
            commands_exec(l->buf);
            out("> ", 2);
        }
    } else if (c == '\b' || c == 0x7F) {
        if (l->len) {
            l->len--;
            out("\b \b", 3);
        }
    } else if (c >= 0x20 && c < 0x7F && l->len < LINE_MAX - 1) {
        l->buf[l->len++] = c;
        out(&c, 1);
    }
}

void console_task(void) {
    for (uint8_t i = 0; i < 2; i++) {
        int c;
        uint8_t budget = 32;
        while (budget-- && (c = lines[i].getc()) >= 0) {
            feed(&lines[i], (char)c);
        }
    }
}
