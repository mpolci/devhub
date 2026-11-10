// Auxiliary relay logic. "Disconnected" means coil energized (COM/NC open).
#ifndef APP_RELAY_H
#define APP_RELAY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void (*set_coil)(bool energized);
    void (*on_state)(uint8_t state, uint8_t origin);
} relay_hw_t;

void relay_init(const relay_hw_t *hw);
void relay_task(uint32_t now);

void relay_set(bool disconnected, uint8_t origin);
// Disconnect the line for `ms`, then reconnect. Returns a protocol status.
uint8_t relay_pulse(uint16_t ms, uint32_t now);
void relay_button(void);

uint8_t relay_state(void);

#endif
