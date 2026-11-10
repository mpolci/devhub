#include <string.h>

#include "app/ports.h"
#include "app/relay.h"
#include "proto/proto.h"
#include "test.h"

// --- Fakes ---------------------------------------------------------------------

static uint32_t now_ms;
static uint8_t defaults_mask;
static bool save_ok;
static int hub_resets, reboots, bootloaders;
static bool coil;

static void set_enabled(uint8_t port, bool on) {}
static void port_state(uint8_t port, uint8_t state, uint8_t origin) {
    proto_event(EVT_PORT_CHANGED, (const uint8_t[]){(uint8_t)(port + 1), state, origin}, 3);
}
static const ports_hw_t ports_hw = {set_enabled, port_state, NULL, NULL};

static void set_coil(bool e) {
    coil = e;
}
static const relay_hw_t relay_hw = {set_coil, NULL};

static uint32_t sys_now(void) {
    return now_ms;
}
static uint8_t sys_flags(void) {
    return SYS_HUB_CONFIGURED;
}
static void sys_uid(uint8_t uid[12]) {
    for (int i = 0; i < 12; i++) {
        uid[i] = (uint8_t)(0xA0 + i);
    }
}
static uint8_t sys_defaults_get(void) {
    return defaults_mask;
}
static void sys_defaults_set(uint8_t m) {
    defaults_mask = m;
}
static bool sys_defaults_save(void) {
    return save_ok;
}
static void sys_hub_reset(void) {
    hub_resets++;
}
static void sys_reboot(void) {
    reboots++;
}
static void sys_bootloader(void) {
    bootloaders++;
}

static const proto_sys_t sys = {sys_now,           sys_flags,     sys_uid,    sys_defaults_get, sys_defaults_set,
                                sys_defaults_save, sys_hub_reset, sys_reboot, sys_bootloader};

static void setup(void) {
    now_ms = 0;
    defaults_mask = 0x0F;
    save_ok = true;
    hub_resets = reboots = bootloaders = 0;
    ports_init(&ports_hw);
    relay_init(&relay_hw);
    proto_init(&sys);
}

static uint8_t seq;

// Send a command; returns the next queued report (or NULL) and pops it.
static const uint8_t *command(uint8_t cmd, const uint8_t *args, uint8_t len) {
    static uint8_t copy[PROTO_REPORT_SIZE];
    uint8_t report[PROTO_REPORT_SIZE] = {REPORT_ID_COMMAND, cmd, ++seq};
    if (len) {
        memcpy(&report[3], args, len);
    }
    proto_handle(report, sizeof report);
    const uint8_t *r = proto_tx_peek();
    if (!r) {
        return NULL;
    }
    memcpy(copy, r, sizeof copy);
    proto_tx_pop();
    return copy;
}

#define ARGS(...) (const uint8_t[]){__VA_ARGS__}, sizeof((const uint8_t[]){__VA_ARGS__})

// Next queued report of the given ID, skipping others.
static const uint8_t *next_of(uint8_t id) {
    static uint8_t copy[PROTO_REPORT_SIZE];
    const uint8_t *r;
    while ((r = proto_tx_peek()) != NULL) {
        memcpy(copy, r, sizeof copy);
        proto_tx_pop();
        if (copy[0] == id) {
            return copy;
        }
    }
    return NULL;
}

// --- Tests ---------------------------------------------------------------------

static void test_get_info_layout(void) {
    setup();
    const uint8_t *r = command(CMD_GET_INFO, NULL, 0);
    CHECK(r != NULL);
    CHECK_EQ(r[0], REPORT_ID_RESPONSE);
    CHECK_EQ(r[1], CMD_GET_INFO);
    CHECK_EQ(r[2], seq);
    CHECK_EQ(r[3], ST_OK);
    CHECK_EQ(r[4], PROTO_VERSION_MAJOR);
    CHECK_EQ(r[5], PROTO_VERSION_MINOR);
    CHECK_EQ(r[9], 4);  // n_ports
    CHECK_EQ(r[10], 1); // n_relays
    CHECK_EQ(r[11], CAP_CDC_CONSOLE);
    CHECK_EQ(r[15], 0xA0); // serial
    CHECK_EQ(r[26], 0xAB);
    CHECK_EQ(r[27], 0);
}

static void test_get_status_layout(void) {
    setup();
    ports_set(1, true, ORIGIN_COMMAND, 0);
    ports_warning(1, PORT_WARN_BACK_VOLTAGE);
    ports_set(2, true, ORIGIN_COMMAND, 200);
    ports_fault(2, PORT_FAULT_OVERCURRENT);
    relay_set(true, ORIGIN_COMMAND);
    while (proto_tx_peek()) {
        proto_tx_pop();
    }
    const uint8_t *r = command(CMD_GET_STATUS, NULL, 0);
    CHECK_EQ(r[3], ST_OK);
    CHECK_EQ(r[4], SYS_HUB_CONFIGURED);
    CHECK_EQ(r[5], 0);                       // port 1 state
    CHECK_EQ(r[8], PORT_STATE_ENABLED);      // port 2 state
    CHECK_EQ(r[10], PORT_WARN_BACK_VOLTAGE); // port 2 warning
    CHECK_EQ(r[11], PORT_STATE_FAULT);       // port 3 state
    CHECK_EQ(r[12], PORT_FAULT_OVERCURRENT); // port 3 fault
    CHECK_EQ(r[17], RELAY_STATE_DISCONNECTED);
}

static void test_set_port_and_event(void) {
    setup();
    const uint8_t *r = command(CMD_SET_PORT, ARGS(2, 1));
    // The state change event is queued before the response.
    CHECK_EQ(r[0], REPORT_ID_EVENT);
    CHECK_EQ(r[1], EVT_PORT_CHANGED);
    CHECK_EQ(r[2], 2);
    CHECK_EQ(r[3], PORT_STATE_ENABLED);
    CHECK_EQ(r[4], ORIGIN_COMMAND);
    r = next_of(REPORT_ID_RESPONSE);
    CHECK(r != NULL && r[1] == CMD_SET_PORT && r[3] == ST_OK);
    CHECK_EQ(ports_state(1), PORT_STATE_ENABLED);
}

static void test_bad_parameters(void) {
    setup();
    CHECK_EQ(command(CMD_SET_PORT, ARGS(0, 1))[3], ST_BAD_PARAM);
    CHECK_EQ(command(CMD_SET_PORT, ARGS(5, 1))[3], ST_BAD_PARAM);
    CHECK_EQ(command(CMD_SET_PORT, ARGS(1, 2))[3], ST_BAD_PARAM);
    CHECK_EQ(command(CMD_CYCLE_PORT, ARGS(1, 0x10, 0x00))[3], ST_BAD_PARAM); // 16 ms
    CHECK_EQ(command(CMD_SET_ALL, ARGS(7))[3], ST_BAD_PARAM);
    CHECK_EQ(command(CMD_SET_RELAY, ARGS(2, 1))[3], ST_BAD_PARAM);
    CHECK_EQ(command(CMD_PULSE_RELAY, ARGS(1, 5, 0))[3], ST_BAD_PARAM);
    CHECK_EQ(command(CMD_SET_DEFAULTS, ARGS(0x10))[3], ST_BAD_PARAM);
    CHECK_EQ(command(CMD_CLEAR_FAULT, ARGS(9))[3], ST_BAD_PARAM);
    CHECK_EQ(command(0x66, NULL, 0)[3], ST_UNKNOWN_CMD);
}

static void test_cycle_command(void) {
    setup();
    const uint8_t *r = command(CMD_CYCLE_PORT, ARGS(1, 0xE8, 0x03)); // 1000 ms, little endian
    r = r[0] == REPORT_ID_RESPONSE ? r : next_of(REPORT_ID_RESPONSE);
    CHECK_EQ(r[3], ST_OK);
    CHECK_EQ(ports_state(0), PORT_STATE_CYCLE);
    ports_task(999);
    CHECK_EQ(ports_state(0), PORT_STATE_CYCLE);
    ports_task(1000);
    CHECK_EQ(ports_state(0), PORT_STATE_ENABLED);
}

static void test_port_fault_status(void) {
    setup();
    ports_set(0, true, ORIGIN_COMMAND, 0);
    ports_fault(0, PORT_FAULT_OVERCURRENT);
    while (proto_tx_peek()) {
        proto_tx_pop();
    }
    CHECK_EQ(command(CMD_CYCLE_PORT, ARGS(1, 0, 0))[3], ST_PORT_FAULT);
    const uint8_t *r = command(CMD_CLEAR_FAULT, ARGS(1));
    r = r[0] == REPORT_ID_RESPONSE ? r : next_of(REPORT_ID_RESPONSE);
    CHECK_EQ(r[3], ST_OK);
    CHECK_EQ(ports_state(0), 0);
}

static void test_relay_commands(void) {
    setup();
    CHECK_EQ(command(CMD_SET_RELAY, ARGS(1, 1))[3], ST_OK);
    CHECK(coil);
    CHECK_EQ(command(CMD_SET_RELAY, ARGS(1, 0))[3], ST_OK);
    CHECK(!coil);
    CHECK_EQ(command(CMD_PULSE_RELAY, ARGS(1, 0xF4, 0x01))[3], ST_OK); // 500 ms
    CHECK(coil);
    relay_task(500);
    CHECK(!coil);
}

static void test_defaults(void) {
    setup();
    CHECK_EQ(command(CMD_GET_DEFAULTS, NULL, 0)[4], 0x0F);
    CHECK_EQ(command(CMD_SET_DEFAULTS, ARGS(0x05))[3], ST_OK);
    CHECK_EQ(command(CMD_GET_DEFAULTS, NULL, 0)[4], 0x05);
    CHECK_EQ(command(CMD_SAVE_DEFAULTS, NULL, 0)[3], ST_OK);
    save_ok = false;
    CHECK_EQ(command(CMD_SAVE_DEFAULTS, NULL, 0)[3], ST_STORAGE_ERROR);
}

static void test_system_commands(void) {
    setup();
    CHECK_EQ(command(CMD_RESET_HUB, NULL, 0)[3], ST_OK);
    CHECK_EQ(hub_resets, 1);
    CHECK(command(CMD_REBOOT, NULL, 0) == NULL); // no response
    CHECK_EQ(reboots, 1);
    CHECK_EQ(command(CMD_ENTER_BOOTLOADER, ARGS(1, 2, 3, 4))[3], ST_BAD_PARAM);
    CHECK_EQ(bootloaders, 0);
    CHECK(command(CMD_ENTER_BOOTLOADER, ARGS('D', 'F', 'U', '!')) == NULL);
    CHECK_EQ(bootloaders, 1);
}

static void test_ignores_other_reports_and_queue_overflow(void) {
    setup();
    uint8_t report[PROTO_REPORT_SIZE] = {REPORT_ID_RESPONSE, CMD_GET_INFO, 1};
    proto_handle(report, sizeof report);
    CHECK(proto_tx_peek() == NULL);
    proto_handle(report, 2); // too short
    CHECK(proto_tx_peek() == NULL);

    for (int i = 0; i < 40; i++) { // more than the queue holds: extra events dropped
        proto_event(EVT_POWER_CHANGED, (const uint8_t[]){(uint8_t)i}, 1);
    }
    int n = 0;
    const uint8_t *r;
    uint8_t first = 0xFF;
    while ((r = proto_tx_peek()) != NULL) {
        if (n == 0) {
            first = r[2];
        }
        n++;
        proto_tx_pop();
    }
    CHECK_EQ(n, 14);
    CHECK_EQ(first, 0); // oldest kept, newest dropped

    // A full event backlog still leaves room for a command response.
    for (int i = 0; i < 40; i++) {
        proto_event(EVT_POWER_CHANGED, (const uint8_t[]){(uint8_t)i}, 1);
    }
    uint8_t cmd[PROTO_REPORT_SIZE] = {REPORT_ID_COMMAND, CMD_GET_DEFAULTS, 9};
    proto_handle(cmd, sizeof cmd);
    r = next_of(REPORT_ID_RESPONSE);
    CHECK(r != NULL && r[1] == CMD_GET_DEFAULTS && r[2] == 9);
}

int main(void) {
    RUN(test_get_info_layout);
    RUN(test_get_status_layout);
    RUN(test_set_port_and_event);
    RUN(test_bad_parameters);
    RUN(test_cycle_command);
    RUN(test_port_fault_status);
    RUN(test_relay_commands);
    RUN(test_defaults);
    RUN(test_system_commands);
    RUN(test_ignores_other_reports_and_queue_overflow);
    return TEST_RESULT();
}
