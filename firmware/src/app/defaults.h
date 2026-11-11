// Settings kept in flash: ports enabled after start-up, ports whose charge counter
// is reset whenever they switch on, port labels.
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
// Port label: up to DEFAULTS_LABEL_LEN printable bytes, "" when not set.
#define DEFAULTS_LABEL_LEN 15
const char *defaults_label_get(uint8_t port);
bool defaults_label_set(uint8_t port, const char *text, uint8_t len); // false if not printable or too long
bool defaults_save(void);
bool defaults_from_flash(void);

#endif
