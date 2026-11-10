#include "app/hub_ctrl.h"

#include <stdio.h>

#include "board/board.h"
#include "console/console.h"
#include "drivers/i2c_bus.h"
#include "drivers/usb2517.h"

#define HUB_VID 0x1209
#define HUB_PID 0x0002 // pid.codes test PID during development
#define HUB_DID 0x0100
#define HUB_PORT_MCU 6
#define HUB_PORTS_DISABLED ((1u << 5) | (1u << 7))

#define HOLD_MS 20             // commanded reset
#define HOLD_UNEXPECTED_MS 1000 // after a reset we did not ask for (D35)
#define RELEASE_TIMEOUT_MS 2000 // supervisor U12 may hold up to 700 ms
#define SETTLE_MS 20
#define CONFIG_ATTEMPTS 3
#define CONFIG_RETRY_MS 50
#define DEFAULTS_SETTLE_MS 100

#define CONFIGURED_MAGIC 0x48554243u // survives MCU resets, not power loss

typedef enum {
    ST_HOLD,         // reset driven low
    ST_WAIT_RELEASE, // released, waiting for the line to actually rise
    ST_SETTLE,       // hub out of reset, waiting before the SMBus download
    ST_RUNNING,
} state_t;

static uint32_t configured_magic __attribute__((section(".noinit")));

static state_t state;
static uint32_t t0;
static uint32_t hold_ms;
static uint8_t attempts;
static bool use_defaults; // fall back to the hub's internal configuration
static bool ready;

static bool elapsed(uint32_t now, uint32_t ms) {
    return (uint32_t)(now - t0) >= ms;
}

static void begin_reset(uint32_t now, uint32_t hold, bool defaults) {
    configured_magic = 0;
    i2c_bus_deinit(I2C_BUS_1); // bus idle: SCL is CFG_SEL0, sampled at release
    board_hub_reset_drive(true);
    board_hub_cfg_sel2(defaults); // low = SMBus download, high = internal defaults
    use_defaults = defaults;
    hold_ms = hold;
    attempts = 0;
    t0 = now;
    state = ST_HOLD;
}

static void run(uint32_t now, bool configured) {
    if (configured) {
        configured_magic = CONFIGURED_MAGIC;
    }
    if (!i2c_bus_ready(I2C_BUS_1)) {
        i2c_bus_init(I2C_BUS_1);
    }
    state = ST_RUNNING;
    ready = true;
    t0 = now;
}

static bool configure(void) {
    static char serial[25];
    uint8_t uid[12];
    board_uid(uid);
    for (uint8_t i = 0; i < 12; i++) {
        snprintf(&serial[2 * i], 3, "%02X", uid[i]);
    }
    const usb2517_config_t cfg = {
        .vid = HUB_VID,
        .pid = HUB_PID,
        .did = HUB_DID,
        // Physical port numbers. To be confirmed on the prototype: the hub
        // renumbers the remaining ports when some are disabled.
        .non_removable = 1u << HUB_PORT_MCU,
        .disabled = HUB_PORTS_DISABLED,
        .manufacturer = "devhub",
        .product = "devhub USB hub",
        .serial = serial,
    };
    return usb2517_configure(&cfg);
}

void hub_ctrl_init(void) {
    uint32_t now = board_millis();
    ready = false;
    if (!board_reset_was_power_on() && configured_magic == CONFIGURED_MAGIC && board_hub_reset_level()) {
        // MCU-only reset: the hub kept its configuration and its USB link.
        LOG("hub: already configured, left untouched");
        run(now, true);
    } else {
        LOG("hub: start-up sequence");
        begin_reset(now, HOLD_MS, false);
    }
}

void hub_ctrl_task(uint32_t now) {
    switch (state) {
    case ST_HOLD:
        if (elapsed(now, hold_ms) && board_i2c1_scl_level()) {
            board_hub_reset_drive(false);
            t0 = now;
            state = ST_WAIT_RELEASE;
        }
        break;

    case ST_WAIT_RELEASE:
        if (board_hub_reset_level()) {
            t0 = now;
            state = ST_SETTLE;
        } else if (elapsed(now, RELEASE_TIMEOUT_MS)) {
            LOG("hub: HUB_RST_N stuck low");
            t0 = now; // keep waiting; nothing else can be done
        }
        break;

    case ST_SETTLE:
        if (use_defaults) {
            if (elapsed(now, DEFAULTS_SETTLE_MS)) {
                LOG("hub: running with internal defaults");
                run(now, false);
            }
            break;
        }
        if (!elapsed(now, attempts ? CONFIG_RETRY_MS : SETTLE_MS)) {
            break;
        }
        i2c_bus_init(I2C_BUS_1);
        if (configure()) {
            LOG("hub: configured over SMBus, attached");
            run(now, true);
        } else if (++attempts >= CONFIG_ATTEMPTS) {
            // The hub would wait forever for the download: restart it with
            // its internal defaults so that USB still works.
            LOG("hub: SMBus configuration failed, falling back to defaults");
            begin_reset(now, HOLD_MS, true);
        } else {
            t0 = now;
        }
        break;

    case ST_RUNNING:
        // Supervisor U12 can reset the hub alone on a 3.3 V dip (D35).
        if (!board_hub_reset_level()) {
            LOG("hub: unexpected reset detected");
            begin_reset(now, HOLD_UNEXPECTED_MS, false);
        }
        break;
    }
}

bool hub_ctrl_ready(void) {
    return ready;
}

bool hub_ctrl_configured(void) {
    return state == ST_RUNNING && configured_magic == CONFIGURED_MAGIC;
}

bool hub_ctrl_i2c1_available(void) {
    return state == ST_RUNNING;
}

void hub_ctrl_request_reset(void) {
    LOG("hub: reset requested");
    begin_reset(board_millis(), HOLD_MS, false);
}

const char *hub_ctrl_state_name(void) {
    switch (state) {
    case ST_HOLD:
        return "reset held";
    case ST_WAIT_RELEASE:
        return "waiting for reset release";
    case ST_SETTLE:
        return "configuring";
    default:
        return hub_ctrl_configured() ? "running (SMBus configuration)" : "running (internal defaults)";
    }
}
