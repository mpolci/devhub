// Port logic: hardware-independent state machine for the four downstream ports.
// Ports are indexed from 0 here; the protocol numbers them from 1.
#ifndef APP_PORTS_H
#define APP_PORTS_H

#include <stdbool.h>
#include <stdint.h>

#include "proto/proto_defs.h"

typedef struct {
    // Switch VBUS and data lines together.
    void (*set_enabled)(uint8_t port, bool on);
    // Called whenever the state byte of a port changes.
    void (*on_state)(uint8_t port, uint8_t state, uint8_t origin);
    void (*on_fault)(uint8_t port, uint8_t cause);
    void (*on_warning)(uint8_t port, uint8_t warning);
} ports_hw_t;

void ports_init(const ports_hw_t *hw);

// Queue the power-on of the ports in `mask` (bit 0 = port 0), staggered.
void ports_startup(uint8_t mask, uint32_t now);

void ports_task(uint32_t now);

// Commands. Return a protocol status code.
uint8_t ports_set(uint8_t port, bool on, uint8_t origin, uint32_t now);
uint8_t ports_cycle(uint8_t port, uint16_t off_ms, uint32_t now);
void ports_set_all(bool on, uint32_t now);
uint8_t ports_clear_fault(uint8_t port);

// Inputs from the board.
void ports_button(uint8_t port, uint32_t now);
void ports_fault(uint8_t port, uint8_t cause);
void ports_warning(uint8_t port, uint8_t warning);

// Queries.
uint8_t ports_state(uint8_t port);
uint8_t ports_fault_cause(uint8_t port);
uint8_t ports_warnings(uint8_t port);
bool ports_led(uint8_t port, uint32_t now);
// True when the port switch is, or is about to be, closed (fault monitoring).
bool ports_is_powered(uint8_t port);

#endif
