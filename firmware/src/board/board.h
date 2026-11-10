// Board support for devhub Rev A (STM32G0B1CEU6). Pin map: docs/pinout-mcu.md.
// Ports and buttons are indexed from 0.
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define BOARD_N_PORTS 4
#define BOARD_N_BUTTONS 5 // 0..3 = ports, 4 = relay
#define BOARD_BUTTON_RELAY 4

// Clocks, safe GPIO start-up, tick. Outputs come up with ports off, data
// lines open and the relay released.
void board_init(void);

uint32_t board_millis(void);
void board_delay_ms(uint32_t ms);

// Port signals.
void board_port_power(uint8_t port, bool on); // PWR_EN of the VBUS switch
void board_port_data(uint8_t port, bool on);  // data switch (OE_N is inverted here)
bool board_port_power_get(uint8_t port);
bool board_port_data_get(uint8_t port);
bool board_port_alert(uint8_t port);          // true = ALERT# asserted (low)
// Ports whose ALERT# had a falling edge since the last call (bit 0 = port 0).
uint8_t board_port_alert_edges(void);

void board_led_port(uint8_t port, bool on);
void board_led_host(bool on);

void board_relay(bool energized);
bool board_relay_get(void);

bool board_button(uint8_t idx); // true = pressed (raw, not debounced)

bool board_efuse_fault(void); // true = EFUSE_FLT_N asserted (low)

// Hub control lines.
void board_hub_reset_drive(bool low); // open-drain: low = hold reset, false = release
bool board_hub_reset_level(void);     // actual level of HUB_RST_N (true = high)
void board_hub_cfg_sel2(bool high);
bool board_i2c1_scl_level(void);

// Upstream VBUS, in millivolts at the connector.
uint16_t board_vbus_det_mv(void);

void board_uid(uint8_t uid[12]);

// Reset cause, sampled once at start-up.
bool board_reset_was_power_on(void);
const char *board_reset_cause(void);

void board_watchdog_start(void);
void board_watchdog_kick(void);

#endif
