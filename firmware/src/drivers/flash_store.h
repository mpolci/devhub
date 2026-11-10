// Single persistent record in the last flash page, protected by a CRC.
#ifndef DRIVERS_FLASH_STORE_H
#define DRIVERS_FLASH_STORE_H

#include <stdbool.h>
#include <stdint.h>

#define FLASH_STORE_MAX_LEN 32

// Returns false if no valid record of this version and length is stored.
bool flash_store_load(uint8_t version, void *data, uint8_t len);
bool flash_store_save(uint8_t version, const void *data, uint8_t len);

#endif
