#include "proto/proto.h"

#include <string.h>

#include "app/ports.h"
#include "app/relay.h"

#ifndef FW_VERSION_MAJOR
#define FW_VERSION_MAJOR 0
#define FW_VERSION_MINOR 0
#define FW_VERSION_PATCH 0
#endif

#define TX_QUEUE_LEN 16
#define TX_RESERVED_FOR_RESPONSES 2 // events never fill the whole queue

static const proto_sys_t *sys;
static uint8_t txq[TX_QUEUE_LEN][PROTO_REPORT_SIZE];
static uint8_t tx_head, tx_count;

static uint8_t *tx_alloc(void) {
    if (tx_count >= TX_QUEUE_LEN) {
        return NULL;
    }
    uint8_t *r = txq[(uint8_t)(tx_head + tx_count) % TX_QUEUE_LEN];
    tx_count++;
    memset(r, 0, PROTO_REPORT_SIZE);
    return r;
}

void proto_init(const proto_sys_t *s) {
    sys = s;
    tx_head = 0;
    tx_count = 0;
}

const uint8_t *proto_tx_peek(void) {
    return tx_count ? txq[tx_head] : NULL;
}

void proto_tx_pop(void) {
    if (tx_count) {
        tx_head = (uint8_t)((tx_head + 1) % TX_QUEUE_LEN);
        tx_count--;
    }
}

void proto_event(uint8_t evt, const uint8_t *data, uint8_t len) {
    if (tx_count >= TX_QUEUE_LEN - TX_RESERVED_FOR_RESPONSES) {
        return;
    }
    uint8_t *r = tx_alloc();
    if (!r) {
        return;
    }
    r[0] = REPORT_ID_EVENT;
    r[1] = evt;
    if (len > PROTO_REPORT_SIZE - 2) {
        len = PROTO_REPORT_SIZE - 2;
    }
    memcpy(&r[2], data, len);
}

static uint16_t get_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

// Protocol port/relay number (from 1) to index; 0xFF if out of range.
static uint8_t index_of(uint8_t n, uint8_t count) {
    return (n >= 1 && n <= count) ? (uint8_t)(n - 1) : 0xFF;
}

void proto_handle(const uint8_t *report, uint16_t len) {
    if (len < 3 || report[0] != REPORT_ID_COMMAND) {
        return;
    }
    uint8_t in[PROTO_REPORT_SIZE] = {0};
    memcpy(in, report, len > PROTO_REPORT_SIZE ? PROTO_REPORT_SIZE : len);

    uint8_t cmd = in[1];
    const uint8_t *arg = &in[3];
    uint32_t now = sys->now();

    uint8_t resp[PROTO_REPORT_SIZE] = {0};
    resp[0] = REPORT_ID_RESPONSE;
    resp[1] = cmd;
    resp[2] = in[2];
    uint8_t *data = &resp[4];
    uint8_t status = ST_OK;
    bool reply = true;

    switch (cmd) {
    case CMD_GET_INFO:
        data[0] = PROTO_VERSION_MAJOR;
        data[1] = PROTO_VERSION_MINOR;
        data[2] = FW_VERSION_MAJOR;
        data[3] = FW_VERSION_MINOR;
        data[4] = FW_VERSION_PATCH;
        data[5] = PROTO_N_PORTS;
        data[6] = PROTO_N_RELAYS;
        put_u32(&data[7], CAP_CDC_CONSOLE);
        sys->get_uid(&data[11]);
        break;

    case CMD_GET_STATUS: {
        uint8_t *p = data;
        *p++ = sys->system_flags();
        for (uint8_t i = 0; i < PROTO_N_PORTS; i++) {
            *p++ = ports_state(i);
            *p++ = ports_fault_cause(i);
            *p++ = ports_warnings(i);
        }
        *p++ = relay_state();
        *p++ = (uint8_t)sys->mcu_temp_c();
        break;
    }

    case CMD_SET_PORT: {
        uint8_t port = index_of(arg[0], PROTO_N_PORTS);
        if (port == 0xFF || arg[1] > 1) {
            status = ST_BAD_PARAM;
        } else {
            status = ports_set(port, arg[1] != 0, ORIGIN_COMMAND, now);
        }
        break;
    }

    case CMD_CYCLE_PORT: {
        uint8_t port = index_of(arg[0], PROTO_N_PORTS);
        status = (port == 0xFF) ? ST_BAD_PARAM : ports_cycle(port, get_u16(&arg[1]), now);
        break;
    }

    case CMD_SET_ALL:
        if (arg[0] > 1) {
            status = ST_BAD_PARAM;
        } else {
            ports_set_all(arg[0] != 0, now);
        }
        break;

    case CMD_SET_RELAY:
        if (index_of(arg[0], PROTO_N_RELAYS) == 0xFF || arg[1] > 1) {
            status = ST_BAD_PARAM;
        } else {
            relay_set(arg[1] != 0, ORIGIN_COMMAND);
        }
        break;

    case CMD_PULSE_RELAY:
        if (index_of(arg[0], PROTO_N_RELAYS) == 0xFF) {
            status = ST_BAD_PARAM;
        } else {
            status = relay_pulse(get_u16(&arg[1]), now);
        }
        break;

    case CMD_GET_DEFAULTS:
        data[0] = sys->defaults_get();
        break;

    case CMD_SET_DEFAULTS:
        if (arg[0] >> PROTO_N_PORTS) {
            status = ST_BAD_PARAM;
        } else {
            sys->defaults_set(arg[0]);
        }
        break;

    case CMD_SAVE_DEFAULTS:
        status = sys->defaults_save() ? ST_OK : ST_STORAGE_ERROR;
        break;

    case CMD_CLEAR_FAULT: {
        uint8_t port = index_of(arg[0], PROTO_N_PORTS);
        status = (port == 0xFF) ? ST_BAD_PARAM : ports_clear_fault(port);
        break;
    }

    case CMD_RESET_HUB:
        sys->request_hub_reset();
        break;

    case CMD_REBOOT:
        sys->request_reboot();
        reply = false;
        break;

    case CMD_ENTER_BOOTLOADER:
        if (get_u32(arg) == BOOTLOADER_MAGIC) {
            sys->request_bootloader();
            reply = false;
        } else {
            status = ST_BAD_PARAM;
        }
        break;

    default:
        status = ST_UNKNOWN_CMD;
        break;
    }

    if (reply) {
        resp[3] = status;
        uint8_t *r = tx_alloc();
        if (r) {
            memcpy(r, resp, PROTO_REPORT_SIZE);
        }
    }
}
