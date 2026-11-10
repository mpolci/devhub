#include "app/portctl.h"

#include "app/defaults.h"
#include "app/hub_ctrl.h"
#include "app/ports.h"
#include "board/board.h"
#include "console/console.h"
#include "drivers/i2c_bus.h"
#include "drivers/ucs2114.h"
#include "proto/proto.h"

#define POLL_MS 100
#define RETRY_MS 1000
// ALERT# level is ignored for this long after a turn-on; edges are not.
#define ALERT_GRACE_MS 20

typedef struct {
    i2c_bus_t bus;
    bool ok;
    uint32_t next_poll;
} chip_t;

static chip_t chips[PORTCTL_N_CHIPS] = {{.bus = I2C_BUS_1}, {.bus = I2C_BUS_2}};
static uint32_t on_time[BOARD_N_PORTS];
static bool started;
static bool led_override;

static bool bus_available(const chip_t *c) {
    return c->bus != I2C_BUS_1 || hub_ctrl_i2c1_available();
}

// --- Callbacks of the port logic --------------------------------------------

static void hw_set_enabled(uint8_t port, bool on) {
    if (on) {
        board_port_power(port, true);
        board_port_data(port, true);
        on_time[port] = board_millis();
    } else {
        board_port_data(port, false);
        board_port_power(port, false);
    }
}

static void hw_on_state(uint8_t port, uint8_t state, uint8_t origin) {
    LOG("port %u: state 0x%02X (origin %u)", port + 1, state, origin);
    proto_event(EVT_PORT_CHANGED, (const uint8_t[]){(uint8_t)(port + 1), state, origin}, 3);
}

static void hw_on_fault(uint8_t port, uint8_t cause) {
    LOG("port %u: FAULT 0x%02X", port + 1, cause);
    proto_event(EVT_PORT_FAULT, (const uint8_t[]){(uint8_t)(port + 1), cause}, 2);
}

static void hw_on_warning(uint8_t port, uint8_t warning) {
    LOG("port %u: warning 0x%02X", port + 1, warning);
    proto_event(EVT_PORT_WARNING, (const uint8_t[]){(uint8_t)(port + 1), warning}, 2);
}

static const ports_hw_t ports_hw = {
    .set_enabled = hw_set_enabled,
    .on_state = hw_on_state,
    .on_fault = hw_on_fault,
    .on_warning = hw_on_warning,
};

// --- Controllers ---------------------------------------------------------------

bool portctl_chip_init(uint8_t chip) {
    chip_t *c = &chips[chip];
    c->ok = false;
    if (!bus_available(c)) {
        return false;
    }
    if (!i2c_bus_ready(c->bus)) {
        i2c_bus_init(c->bus);
    }
    ucs2114_events_t ev;
    if (!ucs2114_check_id(c->bus)) {
        LOG("ucs %u: not found", chip + 1);
    } else if (!ucs2114_read_events(c->bus, &ev) || !ucs2114_configure(c->bus)) {
        LOG("ucs %u: configuration failed", chip + 1);
    } else {
        LOG("ucs %u: configured (discharge 400 ms, ALERT# masked)", chip + 1);
        c->ok = true;
    }
    return c->ok;
}

static uint8_t fault_cause(uint8_t evt) {
    return (uint8_t)(((evt & UCS_EVT_OVERCURRENT) ? PORT_FAULT_OVERCURRENT : 0) |
                     ((evt & UCS_EVT_OVERTEMP) ? PORT_FAULT_OVERTEMP : 0));
}

static uint8_t warning_bits(uint8_t evt) {
    return (uint8_t)(((evt & UCS_EVT_DISCHARGE) ? PORT_WARN_DISCHARGE : 0) |
                     ((evt & UCS_EVT_BACK_VOLTAGE) ? PORT_WARN_BACK_VOLTAGE : 0) |
                     ((evt & UCS_EVT_VBUS_LOW) ? PORT_WARN_VBUS_LOW : 0));
}

// Read and dispatch the events of one chip. `alerted` flags the channels whose
// ALERT# is asserted: they are latched off even if no cause can be read.
// The cause must be read before PWR_EN goes low, which clears the Error state.
static void service_chip(uint8_t chip, uint8_t alerted) {
    chip_t *c = &chips[chip];
    ucs2114_events_t ev = {0};
    bool read = c->ok && bus_available(c) && ucs2114_read_events(c->bus, &ev);

    if (read && ev.was_reset) {
        LOG("ucs %u: reset detected, reconfiguring", chip + 1);
        portctl_chip_init(chip);
    }
    for (uint8_t ch = 0; ch < 2; ch++) {
        uint8_t port = (uint8_t)(chip * 2 + ch);
        uint8_t cause = fault_cause(ev.ch[ch]);
        if (!cause && (alerted & (1u << ch)) && (ev.ch[0] & UCS_EVT_TEMP_WARNING)) {
            cause = PORT_FAULT_OVERTEMP; // first thermal threshold asserting ALERT#
        }
        if (cause || (alerted & (1u << ch))) {
            ports_fault(port, cause); // no readable cause: generic fault
        }
        uint8_t warn = warning_bits(ev.ch[ch]);
        if (warn) {
            ports_warning(port, warn);
        }
    }
    if (!read && c->ok && bus_available(c)) {
        LOG("ucs %u: not responding", chip + 1);
        c->ok = false;
    }
}

void portctl_init(void) {
    ports_init(&ports_hw);
    i2c_bus_init(I2C_BUS_2);
    started = false;
}

void portctl_task(uint32_t now) {
    if (!started) {
        // Bus 1 is free only once the hub start-up sequence has finished.
        if (!hub_ctrl_ready()) {
            return;
        }
        for (uint8_t i = 0; i < PORTCTL_N_CHIPS; i++) {
            portctl_chip_init(i);
            chips[i].next_poll = now + POLL_MS;
        }
        (void)board_port_alert_edges(); // edges seen before ALERT# was masked are stale
        ports_startup(defaults_get(), now);
        started = true;
    }

    // Faults: falling edges latched by the EXTI handler, plus the level for
    // ports that have been on long enough.
    uint8_t alerts = board_port_alert_edges();
    for (uint8_t p = 0; p < BOARD_N_PORTS; p++) {
        if (ports_is_powered(p) && board_port_alert(p) && (uint32_t)(now - on_time[p]) >= ALERT_GRACE_MS) {
            alerts |= (uint8_t)(1u << p);
        }
    }
    for (uint8_t i = 0; i < PORTCTL_N_CHIPS; i++) {
        chip_t *c = &chips[i];
        uint8_t alerted = (alerts >> (2 * i)) & 0x03;
        bool poll = (int32_t)(now - c->next_poll) >= 0;
        if (!alerted && !poll) {
            continue;
        }
        if (poll) {
            c->next_poll = now + (c->ok ? POLL_MS : RETRY_MS);
            if (!c->ok && bus_available(c)) {
                portctl_chip_init(i);
            }
        }
        service_chip(i, alerted);
    }

    ports_task(now);

    if (!led_override) {
        for (uint8_t p = 0; p < BOARD_N_PORTS; p++) {
            board_led_port(p, ports_led(p, now));
        }
    }
}

bool portctl_chip_ok(uint8_t chip) {
    return chip < PORTCTL_N_CHIPS && chips[chip].ok;
}

void portctl_led_override(bool on) {
    led_override = on;
}
