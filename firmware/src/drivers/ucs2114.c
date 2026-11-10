#include "drivers/ucs2114.h"

#include "board/board.h"

#define UCS_ADDR 0x57

#define REG_PORT_STATUS 0x02
#define REG_INT_STATUS2 0x03 // chip port 2
#define REG_INT_STATUS1 0x04 // chip port 1
#define REG_GEN_CFG2 0x11
#define REG_GEN_CFG1 0x12
#define REG_PRODUCT_ID 0xFD
#define REG_MANUFACTURER_ID 0xFE

#define PRODUCT_ID 0xE3
#define MANUFACTURER_ID 0x5D

// Interrupt status bits common to both registers
#define INT_ERR 0x80
#define INT_DISCH_ERR 0x40
#define INT_KEEP_OUT 0x10
#define INT_BACK_V 0x02
#define INT_OV_LIM 0x01
// Interrupt status 1 only
#define INT1_TSD_LOW 0x08
// Interrupt status 2 only
#define INT2_RESET 0x20
#define INT2_TSD_HIGH 0x08

// General configuration
#define CFG_ALERT_MASK 0x80
#define CFG1_EN_VBUS_DISCHG 0x04
#define CFG2_DISCHG_TIME_400MS 0x0C
// Writable bits, for the read-back comparison
#define CFG1_RW_MASK 0xB4
#define CFG2_RW_MASK 0xBC

#define CFG1_VALUE (CFG_ALERT_MASK | CFG1_EN_VBUS_DISCHG)
#define CFG2_VALUE (CFG_ALERT_MASK | CFG2_DISCHG_TIME_400MS)

#define WAKE_DELAY_MS 5    // tSMB_WAKE is 4 ms typical
#define IDLE_SLEEP_MS 150  // tIDLE_SLEEP is 200 ms typical

static uint32_t last_access[I2C_BUS_COUNT];
static bool accessed[I2C_BUS_COUNT];

static void touch(i2c_bus_t bus) {
    last_access[bus] = board_millis();
    accessed[bus] = true;
}

bool ucs2114_wake(i2c_bus_t bus) {
    if (accessed[bus] && (board_millis() - last_access[bus]) < IDLE_SLEEP_MS) {
        return true;
    }
    // The first read after Sleep wakes the chip and returns invalid data.
    uint8_t dummy;
    if (!i2c_read_reg(bus, UCS_ADDR, REG_PORT_STATUS, &dummy, 1)) {
        accessed[bus] = false;
        return false;
    }
    board_delay_ms(WAKE_DELAY_MS);
    touch(bus);
    return true;
}

bool ucs2114_read_reg(i2c_bus_t bus, uint8_t reg, uint8_t *value) {
    if (!ucs2114_wake(bus) || !i2c_read_reg(bus, UCS_ADDR, reg, value, 1)) {
        accessed[bus] = false;
        return false;
    }
    touch(bus);
    return true;
}

bool ucs2114_write_reg(i2c_bus_t bus, uint8_t reg, uint8_t value) {
    if (!ucs2114_wake(bus) || !i2c_write_reg(bus, UCS_ADDR, reg, value)) {
        accessed[bus] = false;
        return false;
    }
    touch(bus);
    return true;
}

bool ucs2114_check_id(i2c_bus_t bus) {
    uint8_t pid, mid;
    return ucs2114_read_reg(bus, REG_PRODUCT_ID, &pid) && ucs2114_read_reg(bus, REG_MANUFACTURER_ID, &mid) &&
           pid == PRODUCT_ID && mid == MANUFACTURER_ID;
}

bool ucs2114_configure(i2c_bus_t bus) {
    uint8_t v1, v2;
    if (!ucs2114_write_reg(bus, REG_GEN_CFG2, CFG2_VALUE) || !ucs2114_write_reg(bus, REG_GEN_CFG1, CFG1_VALUE)) {
        return false;
    }
    if (!ucs2114_read_reg(bus, REG_GEN_CFG2, &v2) || !ucs2114_read_reg(bus, REG_GEN_CFG1, &v1)) {
        return false;
    }
    return (v1 & CFG1_RW_MASK) == CFG1_VALUE && (v2 & CFG2_RW_MASK) == CFG2_VALUE;
}

static uint8_t decode(uint8_t reg) {
    uint8_t e = 0;
    if (reg & INT_OV_LIM) {
        e |= UCS_EVT_OVERCURRENT;
    }
    if (reg & INT_DISCH_ERR) {
        e |= UCS_EVT_DISCHARGE;
    }
    if (reg & INT_BACK_V) {
        e |= UCS_EVT_BACK_VOLTAGE;
    }
    if (reg & INT_KEEP_OUT) {
        e |= UCS_EVT_VBUS_LOW;
    }
    if (reg & INT_ERR) {
        e |= UCS_EVT_ERROR_STATE;
    }
    return e;
}

bool ucs2114_read_events(i2c_bus_t bus, ucs2114_events_t *ev) {
    uint8_t s1, s2;
    if (!ucs2114_read_reg(bus, REG_INT_STATUS1, &s1) || !ucs2114_read_reg(bus, REG_INT_STATUS2, &s2)) {
        return false;
    }
    ev->raw_int1 = s1;
    ev->raw_int2 = s2;
    ev->ch[0] = decode(s1);
    ev->ch[1] = decode(s2);
    ev->was_reset = (s2 & INT2_RESET) != 0;
    if (s1 & INT1_TSD_LOW) {
        ev->ch[0] |= UCS_EVT_TEMP_WARNING;
    }
    if (s2 & INT2_TSD_HIGH) { // die over-temperature: both channels shut down
        ev->ch[0] |= UCS_EVT_OVERTEMP;
        ev->ch[1] |= UCS_EVT_OVERTEMP;
    }
    return true;
}
