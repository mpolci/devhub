#include <string.h>

#include "app/ports.h"
#include "test.h"

static bool enabled[PROTO_N_PORTS];
static int state_events, fault_events, warning_events;
static uint8_t last_state[PROTO_N_PORTS], last_origin[PROTO_N_PORTS];

static void set_enabled(uint8_t port, bool on) {
    enabled[port] = on;
}

static void on_state(uint8_t port, uint8_t state, uint8_t origin) {
    state_events++;
    last_state[port] = state;
    last_origin[port] = origin;
}

static void on_fault(uint8_t port, uint8_t cause) {
    fault_events++;
}

static void on_warning(uint8_t port, uint8_t warning) {
    warning_events++;
}

static const ports_hw_t hw = {set_enabled, on_state, on_fault, on_warning};

static void setup(void) {
    memset(enabled, 0, sizeof enabled);
    state_events = fault_events = warning_events = 0;
    ports_init(&hw);
}

static void run(uint32_t from, uint32_t to) {
    for (uint32_t t = from; t <= to; t++) {
        ports_task(t);
    }
}

static void test_startup_is_staggered(void) {
    setup();
    ports_startup(0x0F, 1000);
    CHECK(enabled[0] && !enabled[1]); // first port immediately
    run(1001, 1099);
    CHECK(!enabled[1]);
    run(1100, 1100);
    CHECK(enabled[1] && !enabled[2]);
    run(1101, 1300);
    CHECK(enabled[2] && enabled[3]);
    CHECK_EQ(last_origin[3], ORIGIN_STARTUP);
    CHECK_EQ(ports_state(3), PORT_STATE_ENABLED);
}

static void test_startup_mask(void) {
    setup();
    ports_startup(0x05, 0);
    run(1, 500);
    CHECK(enabled[0] && !enabled[1] && enabled[2] && !enabled[3]);
}

static void test_set_on_off(void) {
    setup();
    CHECK_EQ(ports_set(1, true, ORIGIN_COMMAND, 10), ST_OK);
    CHECK(enabled[1]);
    CHECK_EQ(ports_state(1), PORT_STATE_ENABLED);
    CHECK_EQ(ports_set(1, false, ORIGIN_COMMAND, 20), ST_OK);
    CHECK(!enabled[1]);
    CHECK_EQ(ports_state(1), 0);
    CHECK_EQ(state_events, 2);
    CHECK_EQ(ports_set(4, true, ORIGIN_COMMAND, 30), ST_BAD_PARAM);
}

static void test_cycle(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    CHECK_EQ(ports_cycle(0, 1000, 5000), ST_OK);
    CHECK(!enabled[0]);
    CHECK_EQ(ports_state(0), PORT_STATE_CYCLE);
    CHECK(!ports_led(0, 5100));
    run(5001, 5999);
    CHECK(!enabled[0]);
    run(6000, 6000);
    CHECK(enabled[0]);
    CHECK_EQ(ports_state(0), PORT_STATE_ENABLED);
    CHECK_EQ(last_origin[0], ORIGIN_TIMER);
}

static void test_cycle_limits_and_default(void) {
    setup();
    CHECK_EQ(ports_cycle(0, 499, 0), ST_BAD_PARAM);
    CHECK_EQ(ports_cycle(0, 60001, 0), ST_BAD_PARAM);
    CHECK_EQ(ports_cycle(0, 0, 0), ST_OK); // 0 = default, 1000 ms
    run(1, 999);
    CHECK(!enabled[0]);
    run(1000, 1000);
    CHECK(enabled[0]); // a cycle on a disabled port ends enabled
}

static void test_cycle_restart(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    ports_cycle(0, 1000, 1000);
    run(1001, 1500);
    ports_cycle(0, 1000, 1500); // restart: off until 2500
    run(1501, 2499);
    CHECK(!enabled[0]);
    run(2500, 2500);
    CHECK(enabled[0]);
}

static void test_button(void) {
    setup();
    ports_button(2, 0);
    CHECK(enabled[2]);
    CHECK_EQ(last_origin[2], ORIGIN_BUTTON);
    ports_button(2, 300);
    CHECK(!enabled[2]);
}

static void test_button_cancels_cycle(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    ports_cycle(0, 1000, 1000);
    ports_button(0, 1200);
    CHECK_EQ(ports_state(0), 0);
    run(1201, 3000);
    CHECK(!enabled[0]); // stays off
    ports_button(0, 3001);
    CHECK(enabled[0]);
}

static void test_fault_latches(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    ports_fault(0, PORT_FAULT_OVERCURRENT);
    CHECK(!enabled[0]);
    CHECK_EQ(ports_state(0), PORT_STATE_FAULT);
    CHECK_EQ(ports_fault_cause(0), PORT_FAULT_OVERCURRENT);
    CHECK_EQ(fault_events, 1);
    CHECK_EQ(last_origin[0], ORIGIN_FAULT);
    run(1, 5000);
    CHECK(!enabled[0]); // no automatic recovery
    CHECK(ports_led(0, 250) != ports_led(0, 500)); // blinking
    CHECK_EQ(ports_cycle(0, 1000, 5000), ST_PORT_FAULT);
    ports_fault(0, PORT_FAULT_OVERCURRENT); // already latched: no new event
    CHECK_EQ(fault_events, 1);
}

static void test_fault_generic_and_ignored_when_off(void) {
    setup();
    ports_fault(1, PORT_FAULT_OVERCURRENT); // port off: ignored
    CHECK_EQ(ports_state(1), 0);
    ports_set(1, true, ORIGIN_COMMAND, 0);
    ports_fault(1, 0);
    CHECK_EQ(ports_fault_cause(1), PORT_FAULT_GENERIC);
}

static void test_fault_retry_and_clear(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    ports_fault(0, PORT_FAULT_OVERTEMP);
    ports_button(0, 1000); // explicit retry from the button
    CHECK(enabled[0]);
    CHECK_EQ(ports_fault_cause(0), 0);
    ports_fault(0, PORT_FAULT_OVERTEMP); // fault persists: latched again
    CHECK_EQ(ports_state(0), PORT_STATE_FAULT);
    CHECK_EQ(ports_clear_fault(0), ST_OK);
    CHECK_EQ(ports_state(0), 0);
    CHECK(!enabled[0]); // clearing does not re-enable
    ports_fault(0, PORT_FAULT_OVERTEMP);
    ports_set(0, true, ORIGIN_COMMAND, 2000); // retry from the PC
    CHECK(enabled[0]);
}

static void test_fault_aborts_cycle(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    ports_cycle(0, 1000, 1000);
    ports_fault(0, PORT_FAULT_OVERTEMP);
    run(1001, 3000);
    CHECK(!enabled[0]);
    CHECK_EQ(ports_state(0), PORT_STATE_FAULT);
}

static void test_set_all(void) {
    setup();
    ports_set(1, true, ORIGIN_COMMAND, 0);
    ports_fault(1, PORT_FAULT_OVERCURRENT);
    ports_set_all(true, 1000);
    run(1001, 1500);
    CHECK(enabled[0] && !enabled[1] && enabled[2] && enabled[3]); // latched port skipped
    CHECK_EQ(ports_state(1), PORT_STATE_FAULT);
    ports_cycle(2, 1000, 2000);
    ports_set_all(false, 2100);
    run(2101, 4000);
    CHECK(!enabled[0] && !enabled[2] && !enabled[3]); // cycle cancelled
    CHECK_EQ(ports_state(1), 0);                      // off clears the latch
}

static void test_warnings(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    ports_cycle(0, 1000, 100);
    ports_warning(0, PORT_WARN_DISCHARGE);
    ports_warning(0, PORT_WARN_DISCHARGE); // same bit: reported once
    CHECK_EQ(warning_events, 1);
    CHECK_EQ(ports_warnings(0), PORT_WARN_DISCHARGE);
    run(101, 1100);
    CHECK(enabled[0]); // a warning does not stop the cycle
    CHECK_EQ(ports_warnings(0), PORT_WARN_DISCHARGE);
    ports_set(0, false, ORIGIN_COMMAND, 2000); // next command clears it
    CHECK_EQ(ports_warnings(0), 0);
}

static void test_tick_wraparound(void) {
    setup();
    uint32_t t = 0xFFFFFF00u;
    ports_set(0, true, ORIGIN_COMMAND, t);
    ports_cycle(0, 1000, t);
    for (uint32_t i = 1; i < 1000; i++) {
        ports_task(t + i);
    }
    CHECK(!enabled[0]);
    ports_task(t + 1000); // past the 32-bit wrap
    CHECK(enabled[0]);
}

int main(void) {
    RUN(test_startup_is_staggered);
    RUN(test_startup_mask);
    RUN(test_set_on_off);
    RUN(test_cycle);
    RUN(test_cycle_limits_and_default);
    RUN(test_cycle_restart);
    RUN(test_button);
    RUN(test_button_cancels_cycle);
    RUN(test_fault_latches);
    RUN(test_fault_generic_and_ignored_when_off);
    RUN(test_fault_retry_and_clear);
    RUN(test_fault_aborts_cycle);
    RUN(test_set_all);
    RUN(test_warnings);
    RUN(test_tick_wraparound);
    return TEST_RESULT();
}
