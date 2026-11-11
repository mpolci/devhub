#include "app/portctl.h"

#include <stddef.h>

#include "app/defaults.h"
#include "app/hub_ctrl.h"
#include "app/ports.h"
#include "board/board.h"
#include "console/console.h"
#include "drivers/i2c_bus.h"
#include "drivers/ucs2114.h"
#include "proto/proto.h"

#define POLL_MS 100
#define CC_POLL_MS 20 // current limiting and total current checks
// Total current (D105): ports plus the devhub itself against the power supply.
#define INTERNAL_MA 480     // hub, MCU, LEDs and an energized relay, worst case
#define BUDGET_HOLD_MS 50   // the total must stay above the supply this long
// LED of a port whose limit was lowered by the total current check: on, with a
// short off pulse every period; distinct from the regular fault blink.
#define REDUCED_LED_PERIOD_MS 1000
#define REDUCED_LED_OFF_MS 100
#define RETRY_MS 1000
// ALERT# level is ignored for this long after a turn-on; edges are not.
#define ALERT_GRACE_MS 20

typedef struct {
    i2c_bus_t bus;
    bool ok;
    uint32_t next_poll;
    uint32_t next_cc_poll;
    const char *last_error;
} chip_t;

static chip_t chips[PORTCTL_N_CHIPS] = {{.bus = I2C_BUS_1}, {.bus = I2C_BUS_2}};
static uint32_t on_time[BOARD_N_PORTS];
static uint32_t cc_since[BOARD_N_PORTS]; // first poll that found the port limiting
static bool cc_active[BOARD_N_PORTS];
// Limit lowered by the total current check, 0 when the configured one applies.
static uint16_t reduced_limit[BOARD_N_PORTS];
static bool over_budget;
static uint32_t over_since;
static bool started;
static bool led_override;

static bool bus_available(const chip_t *c) {
    return c->bus != I2C_BUS_1 || hub_ctrl_i2c1_available();
}

static chip_t *usable_chip(uint8_t port);
static uint16_t effective_limit(uint8_t port);
static bool apply_limits(chip_t *c, uint8_t chip);

// --- Callbacks of the port logic --------------------------------------------

static void hw_set_enabled(uint8_t port, bool on) {
    if (on) {
        board_port_power(port, true);
        board_port_data(port, true);
        on_time[port] = board_millis();
        if (reduced_limit[port]) { // a limit lowered by the total current check lasts until the next turn-on
            reduced_limit[port] = 0;
            chip_t *c = usable_chip(port);
            if (c) {
                apply_limits(c, port / 2);
            }
        }
        if ((defaults_charge_reset_get() >> port) & 1u) {
            portctl_reset_charge(port);
        }
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
    const char *error = NULL;
    if (!ucs2114_check_id(c->bus)) {
        error = "not found";
    } else if (!ucs2114_read_events(c->bus, &ev) || !ucs2114_configure(c->bus) ||
               !apply_limits(c, chip)) {
        error = "configuration failed";
    } else {
        LOG("ucs %u: configured (discharge 400 ms, ALERT# masked, charge accumulation, limits %u/%u mA)", chip + 1,
            effective_limit(chip * 2), effective_limit(chip * 2 + 1));
        c->ok = true;
    }
    // Retried every second: report a failure once, not at every attempt.
    if (error && error != c->last_error) {
        LOG("ucs %u: %s", chip + 1, error);
    }
    c->last_error = error;
    return c->ok;
}

static uint8_t fault_cause(uint8_t evt) {
    return (uint8_t)(((evt & UCS_EVT_OVERCURRENT) ? PORT_FAULT_OVERCURRENT : 0) |
                     ((evt & UCS_EVT_OVERTEMP) ? PORT_FAULT_OVERTEMP : 0));
}

static uint16_t effective_limit(uint8_t port) {
    return reduced_limit[port] ? reduced_limit[port] : defaults_port_limit_get(port);
}

static bool cc_limited(uint8_t port) {
    return effective_limit(port) <= UCS2114_CC_MAX_MA;
}

static bool apply_limits(chip_t *c, uint8_t chip) {
    return ucs2114_set_current_limits(c->bus, effective_limit(chip * 2), effective_limit(chip * 2 + 1));
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
    if (alerted) {
        LOG("ucs %u: ALERT# mask 0x%X, read %d, int1 0x%02X, int2 0x%02X", chip + 1, alerted, read, ev.raw_int1,
            ev.raw_int2);
    }
    for (uint8_t ch = 0; ch < 2; ch++) {
        uint8_t port = (uint8_t)(chip * 2 + ch);
        uint8_t cause = fault_cause(ev.ch[ch]);
        if (!cause && (alerted & (1u << ch)) && (ev.ch[0] & UCS_EVT_TEMP_WARNING)) {
            cause = PORT_FAULT_OVERTEMP; // first thermal threshold asserting ALERT#
        }
        // A port limited in Constant Current mode switched off by VBUS under the
        // minimum is an overload, not a warning.
        if (!cause && cc_limited(port) && (ev.ch[ch] & UCS_EVT_VBUS_LOW)) {
            cause = PORT_FAULT_LIMIT;
        }
        if (cause || (alerted & (1u << ch))) {
            ports_fault(port, cause); // no readable cause: generic fault
        }
        uint8_t warn = warning_bits(ev.ch[ch]);
        if (cause == PORT_FAULT_LIMIT) {
            warn &= (uint8_t)~PORT_WARN_VBUS_LOW;
        }
        if (warn) {
            ports_warning(port, warn);
        }
    }
    if (!read && c->ok && bus_available(c)) {
        LOG("ucs %u: not responding", chip + 1);
        c->ok = false;
    }
}

// Ports with a Constant Current limit: switched off when they stay in current
// limiting longer than their tolerance (D104). Short limiting, as when a device
// charges its capacitors at turn-on or when plugged in, is let through.
static void check_cc(uint8_t chip, uint32_t now) {
    chip_t *c = &chips[chip];
    uint8_t watched = 0;
    for (uint8_t ch = 0; ch < 2; ch++) {
        uint8_t port = (uint8_t)(chip * 2 + ch);
        if (ports_is_powered(port) && cc_limited(port)) {
            watched |= (uint8_t)(1u << ch);
        } else {
            cc_active[port] = false;
        }
    }
    uint8_t mask;
    if (!watched || !c->ok || !bus_available(c) || !ucs2114_read_cc_mode(c->bus, &mask)) {
        return;
    }
    for (uint8_t ch = 0; ch < 2; ch++) {
        uint8_t port = (uint8_t)(chip * 2 + ch);
        if (!(watched & mask & (1u << ch))) {
            cc_active[port] = false;
            continue;
        }
        if (reduced_limit[port]) {
            cc_active[port] = false; // lowered by the total current check: limiting is the point
            continue;
        }
        if (!cc_active[port]) {
            cc_active[port] = true;
            cc_since[port] = now;
        } else if ((uint32_t)(now - cc_since[port]) >= defaults_tolerance_get(port)) {
            LOG("port %u: current limiting for %lu ms", port + 1, (unsigned long)(now - cc_since[port]));
            cc_active[port] = false;
            ports_fault(port, PORT_FAULT_LIMIT);
        }
    }
}

// Total current check (D105): when the ports plus the devhub itself stay above the
// power supply for BUDGET_HOLD_MS, lower the limit of the port drawing most to the
// step that would bring the total back within the supply; a port already at the
// lowest step is switched off and latched instead.
static void check_budget(uint32_t now) {
    uint16_t ma[BOARD_N_PORTS] = {0};
    uint32_t total = INTERNAL_MA;
    for (uint8_t chip = 0; chip < PORTCTL_N_CHIPS; chip++) {
        chip_t *c = &chips[chip];
        uint8_t first = (uint8_t)(chip * 2);
        if (!ports_is_powered(first) && !ports_is_powered(first + 1)) {
            continue;
        }
        if (!c->ok || !bus_available(c) || !ucs2114_read_currents_ma(c->bus, &ma[first], &ma[first + 1])) {
            return; // incomplete reading: no decision
        }
        for (uint8_t i = first; i < first + 2; i++) {
            if (!ports_is_powered(i)) {
                ma[i] = 0;
            }
            total += ma[i];
        }
    }
    uint16_t supply = defaults_supply_get();
    if (total <= supply) {
        over_budget = false;
        return;
    }
    if (!over_budget) {
        over_budget = true;
        over_since = now;
        return;
    }
    if ((uint32_t)(now - over_since) < BUDGET_HOLD_MS) {
        return;
    }
    over_budget = false; // the next action waits for a new BUDGET_HOLD_MS
    uint8_t port = 0;
    for (uint8_t i = 1; i < BOARD_N_PORTS; i++) {
        if (ma[i] > ma[port]) {
            port = i;
        }
    }
    uint32_t excess = total - supply;
    uint16_t target = ma[port] > excess ? (uint16_t)(ma[port] - excess) : 0;
    uint16_t lowered = ucs2114_ilim_round(target); // the lowest step if target is below it
    chip_t *c = usable_chip(port);
    if (lowered < effective_limit(port) && c) {
        LOG("budget: %lu mA over %u mA, port %u limit %u -> %u mA", (unsigned long)total, supply, port + 1,
            effective_limit(port), lowered);
        reduced_limit[port] = lowered;
        cc_active[port] = false;
        if (!apply_limits(c, port / 2)) {
            LOG("ucs %u: current limit not applied", port / 2 + 1);
        }
        ports_warning(port, PORT_WARN_BUDGET);
    } else {
        LOG("budget: %lu mA over %u mA, port %u off", (unsigned long)total, supply, port + 1);
        ports_fault(port, PORT_FAULT_BUDGET);
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
        now = board_millis(); // the controller set-up above took tens of milliseconds
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
    for (uint8_t i = 0; i < PORTCTL_N_CHIPS; i++) {
        if ((int32_t)(now - chips[i].next_cc_poll) >= 0) {
            chips[i].next_cc_poll = now + CC_POLL_MS;
            check_cc(i, now);
        }
    }
    static uint32_t next_budget;
    if ((int32_t)(now - next_budget) >= 0) {
        next_budget = now + CC_POLL_MS;
        check_budget(now);
    }

    ports_task(now);

    if (!led_override) {
        for (uint8_t p = 0; p < BOARD_N_PORTS; p++) {
            bool led = ports_led(p, now);
            if (reduced_limit[p] && ports_is_powered(p)) {
                led = (now % REDUCED_LED_PERIOD_MS) >= REDUCED_LED_OFF_MS;
            }
            board_led_port(p, led);
        }
    }
}

static chip_t *usable_chip(uint8_t port) {
    if (port >= BOARD_N_PORTS) {
        return NULL;
    }
    chip_t *c = &chips[port / 2];
    return (c->ok && bus_available(c)) ? c : NULL;
}

bool portctl_measure(uint8_t port, uint16_t *ma, uint32_t *uah) {
    chip_t *c = usable_chip(port);
    return c && ucs2114_read_current_ma(c->bus, port % 2, ma) && ucs2114_read_charge_uah(c->bus, port % 2, uah);
}

bool portctl_reset_charge(uint8_t port) {
    chip_t *c = usable_chip(port);
    return c && ucs2114_reset_charge(c->bus, port % 2);
}

uint16_t portctl_set_limit(uint8_t port, uint16_t ma, uint16_t tolerance_ms) {
    if (port >= BOARD_N_PORTS) {
        return 0;
    }
    uint16_t limit = ucs2114_ilim_round(ma);
    defaults_port_limit_set(port, limit);
    defaults_tolerance_set(port, tolerance_ms);
    cc_active[port] = false;
    reduced_limit[port] = 0; // a limit set by hand replaces one lowered by the total current check
    chip_t *c = usable_chip(port);
    if (c && !apply_limits(c, port / 2)) {
        LOG("ucs %u: current limit not applied", port / 2 + 1);
    }
    return limit;
}

uint16_t portctl_effective_limit(uint8_t port) {
    return port < BOARD_N_PORTS ? effective_limit(port) : 0;
}

bool portctl_chip_ok(uint8_t chip) {
    return chip < PORTCTL_N_CHIPS && chips[chip].ok;
}

void portctl_led_override(bool on) {
    led_override = on;
}
