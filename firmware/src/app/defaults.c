#include "app/defaults.h"

#include "drivers/flash_store.h"

#define RECORD_VERSION 1

typedef struct {
    uint8_t port_mask;
    uint8_t charge_reset_mask;
} defaults_t;

static defaults_t current;
static bool loaded;

void defaults_init(void) {
    loaded = flash_store_load(RECORD_VERSION, &current, sizeof current);
    if (!loaded) {
        current.port_mask = DEFAULTS_FACTORY_MASK;
        current.charge_reset_mask = 0;
    }
    current.port_mask &= DEFAULTS_FACTORY_MASK;
    current.charge_reset_mask &= DEFAULTS_FACTORY_MASK;
}

uint8_t defaults_get(void) {
    return current.port_mask;
}

void defaults_set(uint8_t mask) {
    current.port_mask = mask & DEFAULTS_FACTORY_MASK;
}

uint8_t defaults_charge_reset_get(void) {
    return current.charge_reset_mask;
}

void defaults_charge_reset_set(uint8_t mask) {
    current.charge_reset_mask = mask & DEFAULTS_FACTORY_MASK;
}

bool defaults_save(void) {
    loaded = flash_store_save(RECORD_VERSION, &current, sizeof current);
    return loaded;
}

bool defaults_from_flash(void) {
    return loaded;
}
