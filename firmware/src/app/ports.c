#include "app/ports.h"

#include <stddef.h>

#define LED_BLINK_HALF_PERIOD_MS 250

typedef enum {
    P_OFF,
    P_ON,
    P_PENDING, // waiting for its turn-on slot (staggered power-on)
    P_CYCLE,   // off phase of a timed cycle
    P_FAULT,   // latched off after a hardware fault
} pstate_t;

typedef struct {
    pstate_t st;
    uint32_t due;      // P_PENDING / P_CYCLE: earliest turn-on time
    bool from_cycle;   // P_PENDING reached from a cycle
    uint8_t origin;    // origin reported when the pending turn-on happens
    uint8_t fault;
    uint8_t warning;
    uint8_t reported;  // last state byte passed to on_state
} port_t;

static const ports_hw_t *hw;
static port_t ports[PROTO_N_PORTS];
static uint32_t last_on;
static bool last_on_valid;

static uint8_t state_byte(const port_t *p) {
    switch (p->st) {
    case P_ON:
        return PORT_STATE_ENABLED;
    case P_CYCLE:
        return PORT_STATE_CYCLE;
    case P_PENDING:
        return p->from_cycle ? PORT_STATE_CYCLE : 0;
    case P_FAULT:
        return PORT_STATE_FAULT;
    default:
        return 0;
    }
}

static void report(uint8_t port, uint8_t origin) {
    port_t *p = &ports[port];
    uint8_t s = state_byte(p);
    if (s != p->reported) {
        p->reported = s;
        if (hw->on_state) {
            hw->on_state(port, s, origin);
        }
    }
}

static void switch_off(uint8_t port) {
    if (ports[port].st == P_ON) {
        hw->set_enabled(port, false);
    }
}

static void turn_on(uint8_t port, uint32_t now) {
    port_t *p = &ports[port];
    hw->set_enabled(port, true);
    p->st = P_ON;
    last_on = now;
    last_on_valid = true;
    report(port, p->origin);
}

static void request_on(uint8_t port, uint8_t origin, uint32_t due, bool from_cycle) {
    port_t *p = &ports[port];
    p->st = from_cycle ? P_CYCLE : P_PENDING;
    p->from_cycle = from_cycle;
    p->origin = origin;
    p->due = due;
}

void ports_init(const ports_hw_t *h) {
    hw = h;
    last_on_valid = false;
    for (uint8_t i = 0; i < PROTO_N_PORTS; i++) {
        ports[i] = (port_t){.st = P_OFF};
    }
}

void ports_startup(uint8_t mask, uint32_t now) {
    for (uint8_t i = 0; i < PROTO_N_PORTS; i++) {
        if ((mask & (1u << i)) && ports[i].st == P_OFF) {
            request_on(i, ORIGIN_STARTUP, now, false);
        }
    }
    ports_task(now);
}

void ports_task(uint32_t now) {
    // A cycle whose off time has elapsed joins the turn-on queue.
    for (uint8_t i = 0; i < PROTO_N_PORTS; i++) {
        port_t *p = &ports[i];
        if (p->st == P_CYCLE && (int32_t)(now - p->due) >= 0) {
            p->st = P_PENDING;
        }
    }
    // One turn-on per stagger slot, oldest request first.
    if (last_on_valid && (uint32_t)(now - last_on) < PORT_STAGGER_MS) {
        return;
    }
    int best = -1;
    for (uint8_t i = 0; i < PROTO_N_PORTS; i++) {
        port_t *p = &ports[i];
        if (p->st != P_PENDING) {
            continue;
        }
        if (best < 0 || (int32_t)(p->due - ports[best].due) < 0) {
            best = i;
        }
    }
    if (best >= 0) {
        turn_on((uint8_t)best, now);
    }
}

uint8_t ports_set(uint8_t port, bool on, uint8_t origin, uint32_t now) {
    if (port >= PROTO_N_PORTS) {
        return ST_BAD_PARAM;
    }
    port_t *p = &ports[port];
    p->warning = 0;
    if (on) {
        if (p->st == P_ON) {
            return ST_OK;
        }
        // Explicit retry after a fault, or a pending/cycling port: (re)queue.
        p->fault = 0;
        request_on(port, origin, now, false);
        ports_task(now);
        report(port, origin);
    } else {
        switch_off(port);
        p->st = P_OFF;
        p->fault = 0;
        report(port, origin);
    }
    return ST_OK;
}

uint8_t ports_cycle(uint8_t port, uint16_t off_ms, uint32_t now) {
    if (port >= PROTO_N_PORTS) {
        return ST_BAD_PARAM;
    }
    if (off_ms == 0) {
        off_ms = PORT_CYCLE_DEFAULT_MS;
    }
    if (off_ms < PORT_CYCLE_MIN_MS || off_ms > PORT_CYCLE_MAX_MS) {
        return ST_BAD_PARAM;
    }
    port_t *p = &ports[port];
    if (p->st == P_FAULT) {
        return ST_PORT_FAULT;
    }
    p->warning = 0;
    switch_off(port);
    request_on(port, ORIGIN_TIMER, now + off_ms, true);
    report(port, ORIGIN_COMMAND);
    return ST_OK;
}

void ports_set_all(bool on, uint32_t now) {
    for (uint8_t i = 0; i < PROTO_N_PORTS; i++) {
        if (on && ports[i].st == P_FAULT) {
            continue; // a latched port needs an explicit per-port retry
        }
        ports_set(i, on, ORIGIN_COMMAND, now);
    }
}

uint8_t ports_clear_fault(uint8_t port) {
    if (port >= PROTO_N_PORTS) {
        return ST_BAD_PARAM;
    }
    port_t *p = &ports[port];
    p->warning = 0;
    if (p->st == P_FAULT) {
        p->st = P_OFF;
        p->fault = 0;
        report(port, ORIGIN_COMMAND);
    }
    return ST_OK;
}

void ports_button(uint8_t port, uint32_t now) {
    if (port >= PROTO_N_PORTS) {
        return;
    }
    switch (ports[port].st) {
    case P_OFF:
    case P_FAULT: // explicit retry
        ports_set(port, true, ORIGIN_BUTTON, now);
        break;
    default: // on, pending or cycling: cancel and leave the port off
        ports_set(port, false, ORIGIN_BUTTON, now);
        break;
    }
}

void ports_fault(uint8_t port, uint8_t cause) {
    if (port >= PROTO_N_PORTS) {
        return;
    }
    port_t *p = &ports[port];
    if (p->st == P_OFF || p->st == P_FAULT) {
        return; // nothing to protect, or already latched
    }
    switch_off(port);
    p->st = P_FAULT;
    p->fault = cause ? cause : PORT_FAULT_GENERIC;
    if (hw->on_fault) {
        hw->on_fault(port, p->fault);
    }
    report(port, ORIGIN_FAULT);
}

void ports_warning(uint8_t port, uint8_t warning) {
    if (port >= PROTO_N_PORTS) {
        return;
    }
    port_t *p = &ports[port];
    uint8_t fresh = (uint8_t)(warning & ~p->warning);
    if (fresh) {
        p->warning |= warning;
        if (hw->on_warning) {
            hw->on_warning(port, p->warning);
        }
    }
}

uint8_t ports_state(uint8_t port) {
    return port < PROTO_N_PORTS ? state_byte(&ports[port]) : 0;
}

uint8_t ports_fault_cause(uint8_t port) {
    return port < PROTO_N_PORTS ? ports[port].fault : 0;
}

uint8_t ports_warnings(uint8_t port) {
    return port < PROTO_N_PORTS ? ports[port].warning : 0;
}

bool ports_led(uint8_t port, uint32_t now) {
    if (port >= PROTO_N_PORTS) {
        return false;
    }
    switch (ports[port].st) {
    case P_ON:
        return true;
    case P_FAULT:
        return ((now / LED_BLINK_HALF_PERIOD_MS) & 1u) != 0;
    default:
        return false;
    }
}

bool ports_is_powered(uint8_t port) {
    return port < PROTO_N_PORTS && ports[port].st == P_ON;
}
