// Power-on defaults: which ports are enabled after start-up.
#ifndef APP_DEFAULTS_H
#define APP_DEFAULTS_H

#include <stdbool.h>
#include <stdint.h>

#define DEFAULTS_FACTORY_MASK 0x0F // all four ports enabled

void defaults_init(void); // load from flash, or fall back to factory
uint8_t defaults_get(void);
void defaults_set(uint8_t mask);
bool defaults_save(void);
bool defaults_from_flash(void);

#endif
