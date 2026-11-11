#include "app/defaults.h"

#include <string.h>

#include "drivers/flash_store.h"

#define RECORD_VERSION 1

typedef struct {
    uint8_t port_mask;
    uint8_t charge_reset_mask;
    char labels[4][DEFAULTS_LABEL_LEN + 1]; // NUL-terminated
} defaults_t;

_Static_assert(sizeof(defaults_t) <= FLASH_STORE_MAX_LEN, "settings record too large");

static defaults_t current;
static bool loaded;

void defaults_init(void) {
    loaded = flash_store_load(RECORD_VERSION, &current, sizeof current);
    if (!loaded) {
        current.port_mask = DEFAULTS_FACTORY_MASK;
        current.charge_reset_mask = 0;
        memset(current.labels, 0, sizeof current.labels);
    }
    for (uint8_t i = 0; i < 4; i++) {
        current.labels[i][DEFAULTS_LABEL_LEN] = '\0';
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

const char *defaults_label_get(uint8_t port) {
    return port < 4 ? current.labels[port] : "";
}

bool defaults_label_set(uint8_t port, const char *text, uint8_t len) {
    if (port >= 4 || len > DEFAULTS_LABEL_LEN) {
        return false;
    }
    for (uint8_t i = 0; i < len; i++) {
        if ((uint8_t)text[i] < 0x20 || text[i] == 0x7F) {
            return false;
        }
    }
    memset(current.labels[port], 0, sizeof current.labels[port]);
    memcpy(current.labels[port], text, len);
    return true;
}

bool defaults_save(void) {
    loaded = flash_store_save(RECORD_VERSION, &current, sizeof current);
    return loaded;
}

bool defaults_from_flash(void) {
    return loaded;
}
