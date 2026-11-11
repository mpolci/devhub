// Application wiring and system services: relay and button binding, protocol
// services, input power monitoring, deferred reboot/bootloader/hub reset.
#ifndef APP_SYSTEM_H
#define APP_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

// Must be the first call in main(): jumps to the ROM bootloader when the
// previous run asked for it.
void system_early(void);

void system_init(void);
void system_task(uint32_t now);

uint8_t system_flags(void);
// MCU die temperature in °C for the protocol; PROTO_TEMP_UNAVAILABLE if it cannot be read.
int8_t system_mcu_temp_c(void);

void system_request_reboot(void);
void system_request_bootloader(void);
void system_request_hub_reset(void);

#endif
