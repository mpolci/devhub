#include "drivers/ucs2114.h"

#include "board/board.h"

#define UCS_ADDR 0x57

#define REG_CURRENT2 0x00 // chip port 2
#define REG_CURRENT1 0x01 // chip port 1
#define REG_PORT_STATUS 0x02
#define REG_INT_STATUS2 0x03 // chip port 2
#define REG_INT_STATUS1 0x04 // chip port 1
#define REG_GEN_CFG2 0x11
#define REG_GEN_CFG1 0x12
#define REG_CURRENT_LIMIT 0x14 // ILIM_PORT1 in bits 5-3, ILIM_PORT2 in bits 2-0
#define REG_TAC2 0x16 // total accumulated charge, 4 bytes, chip port 2
#define REG_TAC1 0x1A // chip port 1
#define REG_RATION_CFG 0x22
#define REG_LIMIT_BEH2 0x23 // current limit behaviour, chip port 2
#define REG_LIMIT_BEH1 0x24 // chip port 1
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

// Charge rationing enabled on both channels with the "Ignore" behaviour and the
// threshold left at its maximum: the chip only accumulates the charge, it never
// switches a port off nor asserts ALERT#.
#define RTN_EN1 0x80
#define RTN_RST1 0x40
#define RTN_BEH1_IGNORE 0x30
#define RTN_EN2 0x08
#define RTN_RST2 0x04
#define RTN_BEH2_IGNORE 0x03
#define RATION_VALUE (RTN_EN1 | RTN_BEH1_IGNORE | RTN_EN2 | RTN_BEH2_IGNORE)

// Measurement scales: 13.3 mA and 0.00367 mAh per LSB.
#define CURRENT_MA_X10_PER_LSB 133u
#define CHARGE_UAH_X100_PER_LSB 367u

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
    uint8_t v1, v2, vr;
    if (!ucs2114_write_reg(bus, REG_GEN_CFG2, CFG2_VALUE) || !ucs2114_write_reg(bus, REG_GEN_CFG1, CFG1_VALUE) ||
        !ucs2114_write_reg(bus, REG_RATION_CFG, RATION_VALUE)) {
        return false;
    }
    if (!ucs2114_read_reg(bus, REG_GEN_CFG2, &v2) || !ucs2114_read_reg(bus, REG_GEN_CFG1, &v1) ||
        !ucs2114_read_reg(bus, REG_RATION_CFG, &vr)) {
        return false;
    }
    return (v1 & CFG1_RW_MASK) == CFG1_VALUE && (v2 & CFG2_RW_MASK) == CFG2_VALUE && vr == RATION_VALUE;
}

bool ucs2114_read_current_ma(i2c_bus_t bus, uint8_t ch, uint16_t *ma) {
    uint8_t raw;
    if (!ucs2114_read_reg(bus, ch == 0 ? REG_CURRENT1 : REG_CURRENT2, &raw)) {
        return false;
    }
    *ma = (uint16_t)((raw * CURRENT_MA_X10_PER_LSB + 5u) / 10u);
    return true;
}

bool ucs2114_read_currents_ma(i2c_bus_t bus, uint16_t *ch0_ma, uint16_t *ch1_ma) {
    uint8_t b[2]; // 00h = chip port 2, 01h = chip port 1
    if (!ucs2114_wake(bus) || !i2c_read_reg(bus, UCS_ADDR, REG_CURRENT2, b, sizeof b)) {
        accessed[bus] = false;
        return false;
    }
    touch(bus);
    *ch1_ma = (uint16_t)((b[0] * CURRENT_MA_X10_PER_LSB + 5u) / 10u);
    *ch0_ma = (uint16_t)((b[1] * CURRENT_MA_X10_PER_LSB + 5u) / 10u);
    return true;
}

static bool read_tac(i2c_bus_t bus, uint8_t reg, uint32_t *raw) {
    uint8_t b[4];
    if (!ucs2114_wake(bus) || !i2c_read_reg(bus, UCS_ADDR, reg, b, sizeof b)) {
        accessed[bus] = false;
        return false;
    }
    touch(bus);
    *raw = (((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3]) >> 6;
    return true;
}

bool ucs2114_read_charge_uah(i2c_bus_t bus, uint8_t ch, uint32_t *uah) {
    // The registers are updated once a second: a block read that straddles an
    // update can mix old and new bytes, so read until two reads agree.
    uint8_t reg = ch == 0 ? REG_TAC1 : REG_TAC2;
    uint32_t a, b;
    if (!read_tac(bus, reg, &a)) {
        return false;
    }
    for (uint8_t tries = 0; tries < 3; tries++) {
        if (!read_tac(bus, reg, &b)) {
            return false;
        }
        if (a == b) {
            *uah = (uint32_t)(((uint64_t)a * CHARGE_UAH_X100_PER_LSB + 50u) / 100u);
            return true;
        }
        a = b;
    }
    return false;
}

bool ucs2114_reset_charge(i2c_bus_t bus, uint8_t ch) {
    // RTN_RST clears the accumulated charge; it must go back to 0 for the
    // accumulation to restart.
    uint8_t rst = ch == 0 ? RTN_RST1 : RTN_RST2;
    return ucs2114_write_reg(bus, REG_RATION_CFG, RATION_VALUE | rst) &&
           ucs2114_write_reg(bus, REG_RATION_CFG, RATION_VALUE);
}

const uint16_t ucs2114_ilim_ma[UCS2114_ILIM_STEPS] = {530, 960, 1070, 1280, 1600, 2130, 2670};

static uint8_t ilim_code(uint16_t ma) {
    uint8_t code = 0;
    while (code + 1 < UCS2114_ILIM_STEPS && ucs2114_ilim_ma[code + 1] <= ma) {
        code++;
    }
    return code;
}

uint16_t ucs2114_ilim_round(uint16_t ma) {
    return ucs2114_ilim_ma[ilim_code(ma)];
}

// IBUS_R2MIN (SEL_R2_IMIN in registers 23h/24h, bits 4-2) for a limit: the next
// step up keeps Constant Current mode with that ceiling; 100 mA forces Trip mode.
#define R2MIN_MASK 0x1C
#define R2MIN_100MA 0x00
#define R2MIN_960MA 0x08
#define R2MIN_1280MA 0x0C
#define R2MIN_1600MA 0x10

static uint8_t r2min_code(uint16_t limit_ma) {
    switch (limit_ma) {
    case 530:
        return R2MIN_960MA;
    case 960:
    case 1070:
        return R2MIN_1280MA;
    case 1280:
        return R2MIN_1600MA;
    default:
        return R2MIN_100MA;
    }
}

uint16_t ucs2114_ceiling_ma(uint16_t limit_ma) {
    switch (r2min_code(limit_ma)) {
    case R2MIN_960MA:
        return 960;
    case R2MIN_1280MA:
        return 1280;
    case R2MIN_1600MA:
        return 1600;
    default:
        return limit_ma;
    }
}

static bool set_r2min(i2c_bus_t bus, uint8_t reg, uint16_t limit_ma) {
    // Read-modify-write: the other bits hold VBUS_MIN and reserved fields.
    uint8_t v, back;
    if (!ucs2114_read_reg(bus, reg, &v)) {
        return false;
    }
    v = (uint8_t)((v & ~R2MIN_MASK) | r2min_code(limit_ma));
    return ucs2114_write_reg(bus, reg, v) && ucs2114_read_reg(bus, reg, &back) && back == v;
}

bool ucs2114_set_current_limits(i2c_bus_t bus, uint16_t ch0_ma, uint16_t ch1_ma) {
    ch0_ma = ucs2114_ilim_round(ch0_ma);
    ch1_ma = ucs2114_ilim_round(ch1_ma);
    uint8_t value = (uint8_t)((ilim_code(ch0_ma) << 3) | ilim_code(ch1_ma));
    uint8_t back;
    return set_r2min(bus, REG_LIMIT_BEH1, ch0_ma) && set_r2min(bus, REG_LIMIT_BEH2, ch1_ma) &&
           ucs2114_write_reg(bus, REG_CURRENT_LIMIT, value) && ucs2114_read_reg(bus, REG_CURRENT_LIMIT, &back) &&
           (back & 0x3F) == value;
}

bool ucs2114_read_cc_mode(i2c_bus_t bus, uint8_t *mask) {
    uint8_t v;
    if (!ucs2114_read_reg(bus, REG_PORT_STATUS, &v)) {
        return false;
    }
    *mask = (uint8_t)(((v & 0x20) ? 1u : 0u) | ((v & 0x10) ? 2u : 0u)); // CC_MODE1, CC_MODE2
    return true;
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
