// Settings kept in flash: ports enabled after start-up, ports whose charge counter
// is reset whenever they switch on.
#ifndef APP_DEFAULTS_H
#define APP_DEFAULTS_H

#include <stdbool.h>
#include <stdint.h>

#define DEFAULTS_FACTORY_MASK 0x0F // all four ports enabled

void defaults_init(void); // load from flash, or fall back to factory
uint8_t defaults_get(void);
void defaults_set(uint8_t mask);
// Ports (bit 0 = port 1) whose accumulated charge is reset at every turn-on.
uint8_t defaults_charge_reset_get(void);
void defaults_charge_reset_set(uint8_t mask);
bool defaults_save(void);
bool defaults_from_flash(void);

#endif
