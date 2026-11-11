// Port control: binds the port logic to the board signals and to the two
// UCS2114 controllers (faults, warnings, LEDs, staggered start-up).
#ifndef APP_PORTCTL_H
#define APP_PORTCTL_H

#include <stdbool.h>
#include <stdint.h>

#define PORTCTL_N_CHIPS 2

void portctl_init(void);
void portctl_task(uint32_t now);

// The controller of ports 1-2 (chip 0) or 3-4 (chip 1) answers and is configured.
bool portctl_chip_ok(uint8_t chip);
// Run the identification and configuration again (console).
bool portctl_chip_init(uint8_t chip);
// Current (mA) and charge delivered since the last reset (µAh) of a port, read from
// its controller. False if the controller cannot be reached.
bool portctl_measure(uint8_t port, uint16_t *ma, uint32_t *uah);
bool portctl_reset_charge(uint8_t port);
// Set the current limit of a port (rounded down to a controller step, at least the
// lowest one) and the time it may stay in current limiting, store them in the
// settings and apply them. Returns the limit applied in mA.
uint16_t portctl_set_limit(uint8_t port, uint16_t ma, uint16_t tolerance_ms);
// Suspend the port logic's control of the LEDs (console LED test).
void portctl_led_override(bool on);

#endif
