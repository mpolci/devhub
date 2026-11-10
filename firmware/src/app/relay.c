#include "app/relay.h"

#include "proto/proto_defs.h"

static const relay_hw_t *hw;
static bool disconnected;
static bool pulsing;
static uint32_t pulse_end;
static uint8_t reported;

static void apply(bool disc, bool pulse, uint8_t origin) {
    if (disc != disconnected) {
        hw->set_coil(disc);
    }
    disconnected = disc;
    pulsing = pulse;
    uint8_t s = relay_state();
    if (s != reported) {
        reported = s;
        if (hw->on_state) {
            hw->on_state(s, origin);
        }
    }
}

void relay_init(const relay_hw_t *h) {
    hw = h;
    disconnected = false;
    pulsing = false;
    reported = 0;
    hw->set_coil(false);
}

void relay_task(uint32_t now) {
    if (pulsing && (int32_t)(now - pulse_end) >= 0) {
        apply(false, false, ORIGIN_TIMER);
    }
}

void relay_set(bool disc, uint8_t origin) {
    apply(disc, false, origin);
}

uint8_t relay_pulse(uint16_t ms, uint32_t now) {
    if (ms < RELAY_PULSE_MIN_MS || ms > RELAY_PULSE_MAX_MS) {
        return ST_BAD_PARAM;
    }
    pulse_end = now + ms;
    apply(true, true, ORIGIN_COMMAND);
    return ST_OK;
}

void relay_button(void) {
    apply(!disconnected, false, ORIGIN_BUTTON);
}

uint8_t relay_state(void) {
    return (uint8_t)((disconnected ? RELAY_STATE_DISCONNECTED : 0) | (pulsing ? RELAY_STATE_PULSE : 0));
}
