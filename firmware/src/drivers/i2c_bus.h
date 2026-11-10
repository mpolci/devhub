// Blocking I2C master access to the two board buses.
#ifndef DRIVERS_I2C_BUS_H
#define DRIVERS_I2C_BUS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    I2C_BUS_1 = 0, // PB8/PB9: hub (0x2C) and port controller U4 (0x57)
    I2C_BUS_2 = 1, // PB10/PB11: port controller U5 (0x57)
    I2C_BUS_COUNT
} i2c_bus_t;

void i2c_bus_init(i2c_bus_t bus);
// Release the pins (inputs with the external pull-ups) and stop the peripheral.
void i2c_bus_deinit(i2c_bus_t bus);
bool i2c_bus_ready(i2c_bus_t bus);

bool i2c_write(i2c_bus_t bus, uint8_t addr7, const uint8_t *data, uint16_t len);
bool i2c_read_reg(i2c_bus_t bus, uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len);
bool i2c_write_reg(i2c_bus_t bus, uint8_t addr7, uint8_t reg, uint8_t value);
bool i2c_probe(i2c_bus_t bus, uint8_t addr7);

#endif
