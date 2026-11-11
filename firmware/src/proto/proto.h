// HID control protocol: command dispatcher and outgoing report queue.
#ifndef PROTO_H
#define PROTO_H

#include <stdbool.h>
#include <stdint.h>

#include "proto/proto_defs.h"

// Services the protocol needs from the rest of the firmware.
typedef struct {
    uint32_t (*now)(void);
    uint8_t (*system_flags)(void);
    int8_t (*mcu_temp_c)(void); // PROTO_TEMP_UNAVAILABLE if it cannot be read
    void (*get_uid)(uint8_t uid[12]);
    // Port (from 0) current in mA and charge in µAh; false if it cannot be read.
    bool (*port_measure)(uint8_t port, uint16_t *ma, uint32_t *uah);
    bool (*port_reset_charge)(uint8_t port);
    uint8_t (*defaults_get)(void);
    void (*defaults_set)(uint8_t mask);
    bool (*defaults_save)(void);
    uint8_t (*charge_reset_get)(void);
    void (*charge_reset_set)(uint8_t mask);
    void (*request_hub_reset)(void);
    void (*request_reboot)(void);
    void (*request_bootloader)(void);
} proto_sys_t;

void proto_init(const proto_sys_t *sys);

// Handle one command report (report ID included). Queues the response, if any.
void proto_handle(const uint8_t *report, uint16_t len);

// Queue an event report. Dropped if the queue is full.
void proto_event(uint8_t evt, const uint8_t *data, uint8_t len);

// Outgoing queue: peek the oldest report (64 bytes, report ID first), then pop.
const uint8_t *proto_tx_peek(void);
void proto_tx_pop(void);

#endif
