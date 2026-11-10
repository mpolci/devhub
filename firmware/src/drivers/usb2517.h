// Microchip USB2517 hub: configuration download over SMBus (address 0x2C).
#ifndef DRIVERS_USB2517_H
#define DRIVERS_USB2517_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t vid, pid, did;
    uint8_t non_removable; // bit n = port n (1..7)
    uint8_t disabled;      // bit n = port n (1..7)
    const char *manufacturer;
    const char *product;
    const char *serial;
} usb2517_config_t;

// Write the whole register set, then issue USB_ATTACH. The hub accepts the
// download only between reset release and attach.
bool usb2517_configure(const usb2517_config_t *cfg);
bool usb2517_present(void);
// Block read of `len` registers (max 32) starting at `reg`.
bool usb2517_read(uint8_t reg, uint8_t *data, uint8_t len);

#endif
