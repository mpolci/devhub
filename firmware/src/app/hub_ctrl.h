// USB2517 hub: reset sequencing, SMBus configuration and reset supervision.
// Owns I2C bus 1 while the hub is in reset (SCL doubles as CFG_SEL0).
#ifndef APP_HUB_CTRL_H
#define APP_HUB_CTRL_H

#include <stdbool.h>
#include <stdint.h>

void hub_ctrl_init(void);
void hub_ctrl_task(uint32_t now);

// The start-up sequence has finished (successfully or not).
bool hub_ctrl_ready(void);
// The hub runs with the configuration written by this firmware.
bool hub_ctrl_configured(void);
// I2C bus 1 may be used by other drivers.
bool hub_ctrl_i2c1_available(void);
// Reset and reconfigure the hub (RESET_HUB command).
void hub_ctrl_request_reset(void);
const char *hub_ctrl_state_name(void);

#endif
