// USB device of the MCU: HID control channel and CDC console.
#ifndef USB_DEV_H
#define USB_DEV_H

#include <stdbool.h>
#include <stdint.h>

#define USB_VID 0x1209
#define USB_PID 0x0001 // pid.codes test PID during development

void usb_dev_init(void);
void usb_dev_task(void);

// True when the host has configured the device and the bus is not suspended.
bool usb_dev_host_active(void);

void usb_cdc_write(const char *data, uint16_t len);
int usb_cdc_getc(void);

#endif
