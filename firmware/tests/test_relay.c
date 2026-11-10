#include "app/relay.h"
#include "proto/proto_defs.h"
#include "test.h"

static bool coil;
static int events;
static uint8_t last_origin;

static void set_coil(bool energized) {
    coil = energized;
}

static void on_state(uint8_t state, uint8_t origin) {
    events++;
    last_origin = origin;
}

static const relay_hw_t hw = {set_coil, on_state};

static void setup(void) {
    coil = true;
    events = 0;
    relay_init(&hw);
}

static void test_init_releases_coil(void) {
    setup();
    CHECK(!coil);
    CHECK_EQ(relay_state(), 0);
}

static void test_set_and_button(void) {
    setup();
    relay_set(true, ORIGIN_COMMAND);
    CHECK(coil);
    CHECK_EQ(relay_state(), RELAY_STATE_DISCONNECTED);
    relay_set(true, ORIGIN_COMMAND); // no change: no event
    CHECK_EQ(events, 1);
    relay_button();
    CHECK(!coil);
    CHECK_EQ(last_origin, ORIGIN_BUTTON);
}

static void test_pulse(void) {
    setup();
    CHECK_EQ(relay_pulse(9, 0), ST_BAD_PARAM);
    CHECK_EQ(relay_pulse(500, 1000), ST_OK);
    CHECK(coil);
    CHECK_EQ(relay_state(), RELAY_STATE_DISCONNECTED | RELAY_STATE_PULSE);
    relay_task(1499);
    CHECK(coil);
    relay_task(1500);
    CHECK(!coil);
    CHECK_EQ(relay_state(), 0);
    CHECK_EQ(last_origin, ORIGIN_TIMER);
}

static void test_pulse_on_disconnected_line_reconnects(void) {
    setup();
    relay_set(true, ORIGIN_COMMAND);
    relay_pulse(100, 0);
    relay_task(100);
    CHECK(!coil);
}

static void test_button_cancels_pulse(void) {
    setup();
    relay_pulse(1000, 0);
    relay_button(); // reconnects now
    CHECK(!coil);
    relay_set(true, ORIGIN_COMMAND);
    relay_task(1000); // the old pulse must not release a later command
    CHECK(coil);
}

int main(void) {
    RUN(test_init_releases_coil);
    RUN(test_set_and_button);
    RUN(test_pulse);
    RUN(test_pulse_on_disconnected_line_reconnects);
    RUN(test_button_cancels_pulse);
    return TEST_RESULT();
}
