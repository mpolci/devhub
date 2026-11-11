// Microchip UCS2114 dual USB port power switch (SMBus address 0x57).
// One chip per I2C bus; channel 0 is the chip's port 1, channel 1 its port 2.
#ifndef DRIVERS_UCS2114_H
#define DRIVERS_UCS2114_H

#include <stdbool.h>
#include <stdint.h>

#include "drivers/i2c_bus.h"

// Decoded interrupt status of one channel.
#define UCS_EVT_OVERCURRENT 0x01
#define UCS_EVT_OVERTEMP 0x02
#define UCS_EVT_DISCHARGE 0x04
#define UCS_EVT_BACK_VOLTAGE 0x08
#define UCS_EVT_VBUS_LOW 0x10
#define UCS_EVT_ERROR_STATE 0x20 // ERR bit: channel in Error state
#define UCS_EVT_TEMP_WARNING 0x40 // TSD_LOW (chip-wide, reported on channel 0)

typedef struct {
    uint8_t ch[2];  // UCS_EVT_* per channel
    bool was_reset; // RESET bit: the chip lost its configuration
    uint8_t raw_int1, raw_int2;
} ucs2114_events_t;

// Wake the chip if it may be asleep (both PWR_EN low and no recent traffic).
bool ucs2114_wake(i2c_bus_t bus);
bool ucs2114_check_id(i2c_bus_t bus);
// Enable automatic VBUS discharge (400 ms), restrict ALERT# to
// overcurrent/overtemperature and start the charge accumulation; verified by read-back.
bool ucs2114_configure(i2c_bus_t bus);
// Port current, 13.3 mA resolution. ch 0 = chip port 1, ch 1 = chip port 2.
bool ucs2114_read_current_ma(i2c_bus_t bus, uint8_t ch, uint16_t *ma);
// Charge delivered since the last reset, in µAh (updated by the chip once a second).
bool ucs2114_read_charge_uah(i2c_bus_t bus, uint8_t ch, uint32_t *uah);
bool ucs2114_reset_charge(i2c_bus_t bus, uint8_t ch);

// Current limit of each channel, below the one set by the COMM_ILIM resistor
// (2.67 A on this board). Steps in mA; the chip cannot go above the resistor value.
#define UCS2114_ILIM_STEPS 7
extern const uint16_t ucs2114_ilim_ma[UCS2114_ILIM_STEPS];
// Largest step not above ma; the lowest step if ma is below it.
uint16_t ucs2114_ilim_round(uint16_t ma);
bool ucs2114_set_current_limits(i2c_bus_t bus, uint16_t ch0_ma, uint16_t ch1_ma);
// Read and clear the interrupt status registers.
bool ucs2114_read_events(i2c_bus_t bus, ucs2114_events_t *ev);
bool ucs2114_read_reg(i2c_bus_t bus, uint8_t reg, uint8_t *value);
bool ucs2114_write_reg(i2c_bus_t bus, uint8_t reg, uint8_t value);

#endif
