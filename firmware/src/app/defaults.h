// Settings kept in flash: ports enabled after start-up, ports whose charge counter
// is reset whenever they switch on, port labels, current limits.
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
// Current limit of each port and current the power supply can deliver, in mA.
#define DEFAULTS_FACTORY_PORT_LIMIT_MA 2670 // set by the COMM_ILIM resistors
#define DEFAULTS_FACTORY_SUPPLY_MA 3000
#define DEFAULTS_FACTORY_TOLERANCE_MS 100 // time a port may stay in current limiting
uint16_t defaults_port_limit_get(uint8_t port);
void defaults_port_limit_set(uint8_t port, uint16_t ma); // already rounded to a controller step
uint16_t defaults_tolerance_get(uint8_t port);
void defaults_tolerance_set(uint8_t port, uint16_t ms);
uint16_t defaults_supply_get(void);
void defaults_supply_set(uint16_t ma);
bool defaults_save(void);
bool defaults_from_flash(void);

#endif
