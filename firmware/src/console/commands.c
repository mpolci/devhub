// Console commands: board bring-up and diagnostics.
#include <stdlib.h>
#include <string.h>

#include "app/buttons.h"
#include "app/defaults.h"
#include "app/hub_ctrl.h"
#include "app/portctl.h"
#include "app/ports.h"
#include "app/relay.h"
#include "app/system.h"
#include "board/board.h"
#include "console/console.h"
#include "drivers/i2c_bus.h"
#include "drivers/ucs2114.h"
#include "drivers/usb2517.h"
#include "stm32g0xx_hal.h"
#include "usb/usb_dev.h"

#define MAX_ARGS 5
#define P console_printf

typedef struct {
    const char *name;
    const char *usage;
    void (*fn)(int argc, char **argv);
} command_t;

static bool eq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

// Parse "1".."max" into a zero-based index; -1 if invalid.
static int index_arg(const char *s, int max) {
    int n = atoi(s);
    return (n >= 1 && n <= max) ? n - 1 : -1;
}

static int on_off(const char *s) {
    if (eq(s, "on") || eq(s, "1")) {
        return 1;
    }
    if (eq(s, "off") || eq(s, "0")) {
        return 0;
    }
    return -1;
}

static void usage_error(void) {
    P("invalid arguments, see 'help'\r\n");
}

static void cmd_version(int argc, char **argv) {
    uint8_t uid[12];
    board_uid(uid);
    P("devhub firmware %d.%d.%d, protocol %d.%d\r\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH,
      PROTO_VERSION_MAJOR, PROTO_VERSION_MINOR);
    P("uid ");
    for (uint8_t i = 0; i < 12; i++) {
        P("%02X", uid[i]);
    }
    P("\r\nreset cause: %s, uptime %lu ms\r\n", board_reset_cause(), (unsigned long)board_millis());
}

static void cmd_status(int argc, char **argv) {
    P("system flags 0x%02X, usb host %s, vbus upstream %u mV\r\n", system_flags(),
      usb_dev_host_active() ? "active" : "absent", board_vbus_det_mv());
    P("hub: %s\r\n", hub_ctrl_state_name());
    for (uint8_t p = 0; p < BOARD_N_PORTS; p++) {
        P("port %u: state 0x%02X fault 0x%02X warning 0x%02X | EN=%d DATA=%d ALERT#=%s\r\n", p + 1, ports_state(p),
          ports_fault_cause(p), ports_warnings(p), board_port_power_get(p), board_port_data_get(p),
          board_port_alert(p) ? "low" : "high");
    }
    P("relay: state 0x%02X (coil %s)\r\n", relay_state(), board_relay_get() ? "energized" : "released");
    P("efuse fault: %s, defaults mask 0x%X (%s)\r\n", board_efuse_fault() ? "yes" : "no", defaults_get(),
      defaults_from_flash() ? "flash" : "factory");
}

static void cmd_led(int argc, char **argv) {
    // led auto | led host on|off | led <n> on|off
    if (argc == 2 && eq(argv[1], "auto")) {
        portctl_led_override(false);
        return;
    }
    int v = argc == 3 ? on_off(argv[2]) : -1;
    if (v < 0) {
        usage_error();
        return;
    }
    if (eq(argv[1], "host")) {
        board_led_host(v); // overwritten by the USB task at the next loop
        P("note: the host LED follows the USB state\r\n");
        return;
    }
    int n = index_arg(argv[1], BOARD_N_PORTS);
    if (n < 0) {
        usage_error();
        return;
    }
    portctl_led_override(true);
    board_led_port((uint8_t)n, v);
}

static void cmd_btn(int argc, char **argv) {
    for (uint8_t i = 0; i < BOARD_N_BUTTONS; i++) {
        P("button %s%c: %s, %u presses\r\n", i == BOARD_BUTTON_RELAY ? "relay" : "port ",
          i == BOARD_BUTTON_RELAY ? ' ' : (char)('1' + i), buttons_pressed(i) ? "pressed" : "released",
          buttons_count(i));
    }
}

static void cmd_relay(int argc, char **argv) {
    // relay connect|disconnect|pulse <ms>
    if (argc == 2 && eq(argv[1], "connect")) {
        relay_set(false, ORIGIN_COMMAND);
    } else if (argc == 2 && eq(argv[1], "disconnect")) {
        relay_set(true, ORIGIN_COMMAND);
    } else if (argc == 3 && eq(argv[1], "pulse")) {
        if (relay_pulse((uint16_t)atoi(argv[2]), board_millis()) != ST_OK) {
            P("pulse length out of range (%d-%d ms)\r\n", RELAY_PULSE_MIN_MS, RELAY_PULSE_MAX_MS);
        }
    } else {
        usage_error();
    }
}

static void cmd_gpio(int argc, char **argv) {
    // gpio en|data <n> 0|1 : drive one port signal directly, bypassing the port logic
    int n = argc == 4 ? index_arg(argv[2], BOARD_N_PORTS) : -1;
    int v = argc == 4 ? on_off(argv[3]) : -1;
    if (n < 0 || v < 0) {
        usage_error();
        return;
    }
    if (eq(argv[1], "en")) {
        board_port_power((uint8_t)n, v);
    } else if (eq(argv[1], "data")) {
        board_port_data((uint8_t)n, v);
    } else {
        usage_error();
        return;
    }
    P("raw signal set; the port logic state is unchanged\r\n");
}

static void cmd_port(int argc, char **argv) {
    // port <n> on|off|cycle [ms]|clear
    int n = argc >= 3 ? index_arg(argv[1], BOARD_N_PORTS) : -1;
    if (n < 0) {
        usage_error();
        return;
    }
    uint32_t now = board_millis();
    uint8_t st;
    if (eq(argv[2], "on")) {
        st = ports_set((uint8_t)n, true, ORIGIN_COMMAND, now);
    } else if (eq(argv[2], "off")) {
        st = ports_set((uint8_t)n, false, ORIGIN_COMMAND, now);
    } else if (eq(argv[2], "cycle")) {
        st = ports_cycle((uint8_t)n, argc >= 4 ? (uint16_t)atoi(argv[3]) : 0, now);
    } else if (eq(argv[2], "clear")) {
        st = ports_clear_fault((uint8_t)n);
    } else {
        usage_error();
        return;
    }
    if (st != ST_OK) {
        P("refused, status 0x%02X\r\n", st);
    }
}

static void cmd_all(int argc, char **argv) {
    int v = argc == 2 ? on_off(argv[1]) : -1;
    if (v < 0) {
        usage_error();
        return;
    }
    ports_set_all(v, board_millis());
}

static void cmd_hub(int argc, char **argv) {
    // hub status|reset|dump
    if (argc == 2 && eq(argv[1], "reset")) {
        hub_ctrl_request_reset();
    } else if (argc == 2 && eq(argv[1], "dump")) {
        // Only answers before USB attach; afterwards the SMBus interface is closed.
        uint8_t regs[22];
        if (!hub_ctrl_i2c1_available() || !usb2517_read(0x00, regs, sizeof regs)) {
            P("hub registers not readable (SMBus closed after attach, or bus busy)\r\n");
            return;
        }
        for (uint8_t i = 0; i < sizeof regs; i++) {
            P("%02X%s", regs[i], (i % 8 == 7) ? "\r\n" : " ");
        }
        P("\r\n");
    } else {
        P("hub: %s, HUB_RST_N %s, SMBus ack %s\r\n", hub_ctrl_state_name(),
          board_hub_reset_level() ? "high" : "low",
          hub_ctrl_i2c1_available() && usb2517_present() ? "yes" : "no");
    }
}

static void cmd_ucs(int argc, char **argv) {
    // ucs <1|2> id|init|status|dump | ucs <1|2> write <hex reg> <hex value>
    int chip = argc >= 3 ? index_arg(argv[1], PORTCTL_N_CHIPS) : -1;
    if (chip < 0) {
        usage_error();
        return;
    }
    i2c_bus_t bus = chip == 0 ? I2C_BUS_1 : I2C_BUS_2;
    if (bus == I2C_BUS_1 && !hub_ctrl_i2c1_available()) {
        P("I2C bus 1 is held by the hub reset sequence\r\n");
        return;
    }
    if (argc == 5 && eq(argv[2], "write")) {
        uint8_t reg = (uint8_t)strtoul(argv[3], NULL, 16), value = (uint8_t)strtoul(argv[4], NULL, 16), back = 0;
        bool ok = ucs2114_write_reg(bus, reg, value) && ucs2114_read_reg(bus, reg, &back);
        P("ucs %d: %02Xh <- %02X, %s, reads back %02X\r\n", chip + 1, reg, value, ok ? "ok" : "FAILED", back);
    } else if (argc != 3) {
        usage_error();
    } else if (eq(argv[2], "id")) {
        P("ucs %d: %s\r\n", chip + 1, ucs2114_check_id(bus) ? "UCS2114 found" : "not found");
    } else if (eq(argv[2], "init")) {
        P("ucs %d: %s\r\n", chip + 1, portctl_chip_init((uint8_t)chip) ? "configured" : "failed");
    } else if (eq(argv[2], "status")) {
        ucs2114_events_t ev;
        uint8_t st = 0;
        if (!ucs2114_read_events(bus, &ev) || !ucs2114_read_reg(bus, 0x02, &st)) {
            P("ucs %d: read failed\r\n", chip + 1);
            return;
        }
        P("ucs %d: port status 0x%02X, int1 0x%02X, int2 0x%02X%s (events are cleared by this read)\r\n", chip + 1,
          st, ev.raw_int1, ev.raw_int2, ev.was_reset ? ", RESET" : "");
    } else if (eq(argv[2], "dump")) {
        static const uint8_t regs[] = {0x00, 0x01, 0x02, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
                                       0x15, 0x22, 0x23, 0x24, 0xFD, 0xFE, 0xFF};
        for (uint8_t i = 0; i < sizeof regs; i++) {
            uint8_t v;
            if (ucs2114_read_reg(bus, regs[i], &v)) {
                P("%02Xh=%02X ", regs[i], v);
            } else {
                P("%02Xh=?? ", regs[i]);
            }
        }
        P("\r\n(interrupt status 03h/04h not read: use 'status')\r\n");
    } else {
        usage_error();
    }
}

static void cmd_io(int argc, char **argv) {
    // Raw GPIO registers, for checking levels and pin modes.
    static const struct {
        char name;
        GPIO_TypeDef *port;
    } gpios[] = {{'A', GPIOA}, {'B', GPIOB}, {'C', GPIOC}, {'D', GPIOD}, {'F', GPIOF}};
    for (uint8_t i = 0; i < sizeof gpios / sizeof gpios[0]; i++) {
        const GPIO_TypeDef *g = gpios[i].port;
        P("GPIO%c IDR=%04lX ODR=%04lX MODER=%08lX PUPDR=%08lX\r\n", gpios[i].name, (unsigned long)g->IDR,
          (unsigned long)g->ODR, (unsigned long)g->MODER, (unsigned long)g->PUPDR);
    }
}

static void cmd_pin(int argc, char **argv) {
    // pin <port letter><number>, e.g. "pin B0": read the pin as an input with
    // the internal pull-up and then the pull-down, and restore its mode. A pin
    // that follows both pulls is floating (unsoldered or undriven net).
    static const char letters[] = "ABCDF";
    GPIO_TypeDef *const ports[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOF};
    const char *l = argc == 2 ? strchr(letters, argv[1][0]) : NULL;
    int n = l ? atoi(&argv[1][1]) : -1;
    if (!l || !*l || n < 0 || n > 15) {
        usage_error();
        return;
    }
    GPIO_TypeDef *g = ports[l - letters];
    uint32_t moder = g->MODER, pupdr = g->PUPDR, mask = 3u << (2 * n);
    int level = (g->IDR >> n) & 1;
    g->MODER = moder & ~mask; // input
    g->PUPDR = (pupdr & ~mask) | (1u << (2 * n));
    board_delay_ms(2);
    int up = (g->IDR >> n) & 1;
    g->PUPDR = (pupdr & ~mask) | (2u << (2 * n));
    board_delay_ms(2);
    int down = (g->IDR >> n) & 1;
    g->PUPDR = pupdr;
    g->MODER = moder;
    P("P%c%d: level %d, with pull-up %d, with pull-down %d -> %s\r\n", *l, n, level, up, down,
      up != down ? "floating" : up ? "held high" : "held low");
}

static void cmd_i2c(int argc, char **argv) {
    // i2c <1|2> scan
    int bus = argc == 3 ? index_arg(argv[1], I2C_BUS_COUNT) : -1;
    if (bus < 0 || !eq(argv[2], "scan")) {
        usage_error();
        return;
    }
    if (bus == I2C_BUS_1 && !hub_ctrl_i2c1_available()) {
        P("I2C bus 1 is held by the hub reset sequence\r\n");
        return;
    }
    P("bus %d:", bus + 1);
    uint8_t found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_probe((i2c_bus_t)bus, addr)) {
            P(" 0x%02X", addr);
            found++;
        }
        board_watchdog_kick();
    }
    P("%s\r\n", found ? "" : " no device answers");
}

static void cmd_defaults(int argc, char **argv) {
    // defaults | defaults set <hex mask> | defaults save
    if (argc == 3 && eq(argv[1], "set")) {
        defaults_set((uint8_t)strtoul(argv[2], NULL, 16));
    } else if (argc == 2 && eq(argv[1], "save")) {
        P("%s\r\n", defaults_save() ? "saved" : "flash write failed");
        return;
    } else if (argc != 1) {
        usage_error();
        return;
    }
    P("defaults mask 0x%X (%s)\r\n", defaults_get(), defaults_from_flash() ? "flash" : "not saved");
}

static void cmd_log(int argc, char **argv) {
    console_dump_history();
}

static void cmd_reset(int argc, char **argv) {
    system_request_reboot();
}

static void cmd_dfu(int argc, char **argv) {
    P("entering the ROM bootloader\r\n");
    system_request_bootloader();
}

static void cmd_help(int argc, char **argv);

static const command_t commands[] = {
    {"help", "", cmd_help},
    {"version", "", cmd_version},
    {"status", "", cmd_status},
    {"log", "(log lines since start-up)", cmd_log},
    {"port", "<n> on|off|cycle [ms]|clear", cmd_port},
    {"all", "on|off", cmd_all},
    {"relay", "connect|disconnect|pulse <ms>", cmd_relay},
    {"defaults", "[set <hex mask>|save]", cmd_defaults},
    {"hub", "[status|reset|dump]", cmd_hub},
    {"ucs", "<1|2> id|init|status|dump|write <reg> <val>", cmd_ucs},
    {"led", "<n>|host on|off, led auto", cmd_led},
    {"btn", "", cmd_btn},
    {"gpio", "en|data <n> 0|1 (raw, bypasses the port logic)", cmd_gpio},
    {"io", "(raw GPIO registers)", cmd_io},
    {"pin", "<A-F><0-15> (pull-up/pull-down test of one pin)", cmd_pin},
    {"i2c", "<1|2> scan", cmd_i2c},
    {"reset", "", cmd_reset},
    {"dfu", "", cmd_dfu},
};

static void cmd_help(int argc, char **argv) {
    for (uint8_t i = 0; i < sizeof commands / sizeof commands[0]; i++) {
        P("  %s %s\r\n", commands[i].name, commands[i].usage);
    }
}

void commands_exec(char *line) {
    char *argv[MAX_ARGS];
    int argc = 0;
    for (char *tok = strtok(line, " \t"); tok && argc < MAX_ARGS; tok = strtok(NULL, " \t")) {
        argv[argc++] = tok;
    }
    if (argc == 0) {
        return;
    }
    for (uint8_t i = 0; i < sizeof commands / sizeof commands[0]; i++) {
        if (eq(argv[0], commands[i].name)) {
            commands[i].fn(argc, argv);
            return;
        }
    }
    P("unknown command '%s', see 'help'\r\n", argv[0]);
}
