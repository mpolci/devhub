#include "drivers/usb2517.h"

#include <string.h>

#include "drivers/i2c_bus.h"

#define HUB_ADDR 0x2C
#define HUB_BUS I2C_BUS_1
#define BLOCK_MAX 32

#define REG_VID_LSB 0x00
#define REG_MFR_STR 0x16
#define REG_PROD_STR 0x54
#define REG_SER_STR 0x92
#define REG_STATUS_CMD 0xFF
#define STR_MAX_CHARS 31 // 62 bytes of UTF-16LE per string

#define CFG1_SELF_POWERED 0x80
#define CFG1_MTT_ENABLE 0x10
#define CFG1_EOP_DISABLE 0x08
#define CFG1_OC_INDIVIDUAL 0x02
#define CFG1_PWR_INDIVIDUAL 0x01
#define CFG2_OC_TIMER_8MS 0x20
#define CFG2_COMPOUND 0x08
#define CFG3_STRING_EN 0x01
#define CMD_USB_ATTACH 0x01

// SMBus block write: register, byte count, data.
static bool block_write(uint8_t reg, const uint8_t *data, uint8_t len) {
    uint8_t buf[2 + BLOCK_MAX];
    while (len) {
        uint8_t n = len > BLOCK_MAX ? BLOCK_MAX : len;
        buf[0] = reg;
        buf[1] = n;
        memcpy(&buf[2], data, n);
        if (!i2c_write(HUB_BUS, HUB_ADDR, buf, (uint16_t)(n + 2))) {
            return false;
        }
        reg = (uint8_t)(reg + n);
        data += n;
        len = (uint8_t)(len - n);
    }
    return true;
}

static uint8_t utf16(const char *s, uint8_t *out) {
    uint8_t chars = 0;
    while (s && s[chars] && chars < STR_MAX_CHARS) {
        out[2 * chars] = (uint8_t)s[chars];
        out[2 * chars + 1] = 0;
        chars++;
    }
    return chars;
}

bool usb2517_present(void) {
    return i2c_probe(HUB_BUS, HUB_ADDR);
}

bool usb2517_read(uint8_t reg, uint8_t *data, uint8_t len) {
    // SMBus block read: the hub returns the byte count first.
    uint8_t buf[1 + BLOCK_MAX];
    if (len > BLOCK_MAX || !i2c_read_reg(HUB_BUS, HUB_ADDR, reg, buf, (uint16_t)(len + 1))) {
        return false;
    }
    memcpy(data, &buf[1], len);
    return true;
}

bool usb2517_configure(const usb2517_config_t *cfg) {
    uint8_t mfr[2 * STR_MAX_CHARS], prod[2 * STR_MAX_CHARS], ser[2 * STR_MAX_CHARS];
    uint8_t mfr_len = utf16(cfg->manufacturer, mfr);
    uint8_t prod_len = utf16(cfg->product, prod);
    uint8_t ser_len = utf16(cfg->serial, ser);
    bool strings = mfr_len || prod_len || ser_len;

    const uint8_t regs[] = {
        (uint8_t)cfg->vid, (uint8_t)(cfg->vid >> 8),   // 00h VID
        (uint8_t)cfg->pid, (uint8_t)(cfg->pid >> 8),   // 02h PID
        (uint8_t)cfg->did, (uint8_t)(cfg->did >> 8),   // 04h DID
        CFG1_SELF_POWERED | CFG1_MTT_ENABLE | CFG1_EOP_DISABLE | CFG1_OC_INDIVIDUAL | CFG1_PWR_INDIVIDUAL, // 06h
        (uint8_t)(CFG2_OC_TIMER_8MS | (cfg->non_removable ? CFG2_COMPOUND : 0)),                           // 07h
        strings ? CFG3_STRING_EN : 0, // 08h
        cfg->non_removable,           // 09h
        cfg->disabled,                // 0Ah port disable, self-powered
        cfg->disabled,                // 0Bh port disable, bus-powered
        0x01,                         // 0Ch max power, self-powered (2 mA)
        0x32,                         // 0Dh max power, bus-powered (100 mA)
        0x01,                         // 0Eh hub controller max current, self-powered
        0x32,                         // 0Fh hub controller max current, bus-powered
        0x32,                         // 10h power-on time (100 ms)
        0x04, 0x09,                   // 11h language ID 0x0409
        mfr_len, prod_len, ser_len,   // 13h string lengths, in characters
    };

    return block_write(REG_VID_LSB, regs, sizeof regs) &&
           (!mfr_len || block_write(REG_MFR_STR, mfr, (uint8_t)(2 * mfr_len))) &&
           (!prod_len || block_write(REG_PROD_STR, prod, (uint8_t)(2 * prod_len))) &&
           (!ser_len || block_write(REG_SER_STR, ser, (uint8_t)(2 * ser_len))) &&
           block_write(REG_STATUS_CMD, (const uint8_t[]){CMD_USB_ATTACH}, 1);
}
