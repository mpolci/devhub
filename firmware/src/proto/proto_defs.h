// Control protocol constants (docs/protocollo-controllo.md, version 0.1).
#ifndef PROTO_DEFS_H
#define PROTO_DEFS_H

#define PROTO_VERSION_MAJOR 0
#define PROTO_VERSION_MINOR 1

#define PROTO_REPORT_SIZE 64 // on the wire, report ID included

#define PROTO_N_PORTS 4
#define PROTO_N_RELAYS 1

// Report IDs
#define REPORT_ID_COMMAND 0x01
#define REPORT_ID_RESPONSE 0x02
#define REPORT_ID_EVENT 0x03

// Commands
#define CMD_GET_INFO 0x01
#define CMD_GET_STATUS 0x02
#define CMD_SET_PORT 0x10
#define CMD_CYCLE_PORT 0x11
#define CMD_SET_ALL 0x12
#define CMD_RESET_CHARGE 0x13
#define CMD_SET_RELAY 0x20
#define CMD_PULSE_RELAY 0x21
#define CMD_GET_DEFAULTS 0x30
#define CMD_SET_DEFAULTS 0x31
#define CMD_SAVE_DEFAULTS 0x32
#define CMD_GET_CHARGE_RESET 0x33
#define CMD_SET_CHARGE_RESET 0x34
#define CMD_GET_LABEL 0x35
#define CMD_SET_LABEL 0x36
#define CMD_GET_LIMITS 0x37
#define CMD_SET_PORT_LIMIT 0x38
#define CMD_SET_SUPPLY_LIMIT 0x39
#define PORT_LIMIT_MIN_MA 530 // lowest step of the port controllers
#define TOLERANCE_MIN_MS 20
#define TOLERANCE_MAX_MS 10000
#define SUPPLY_LIMIT_MIN_MA 1000
#define SUPPLY_LIMIT_MAX_MA 6000
#define PROTO_LABEL_FIELD 16 // label bytes in GET_LABEL/SET_LABEL, NUL-padded
#define CMD_CLEAR_FAULT 0x40
#define CMD_RESET_HUB 0x50
#define CMD_REBOOT 0x7E
#define CMD_ENTER_BOOTLOADER 0x7F

#define BOOTLOADER_MAGIC 0x21554644u // "DFU!" little endian

// Status codes
#define ST_OK 0x00
#define ST_UNKNOWN_CMD 0x01
#define ST_BAD_PARAM 0x02
#define ST_PORT_FAULT 0x03
#define ST_HW_ERROR 0x04
#define ST_STORAGE_ERROR 0x05

// Events
#define EVT_PORT_FAULT 0x80
#define EVT_PORT_CHANGED 0x81
#define EVT_POWER_CHANGED 0x82
#define EVT_RELAY_CHANGED 0x83
#define EVT_PORT_WARNING 0x84

// Event origin
#define ORIGIN_COMMAND 0
#define ORIGIN_BUTTON 1
#define ORIGIN_TIMER 2 // end of cycle or pulse
#define ORIGIN_FAULT 3
#define ORIGIN_STARTUP 4

// Port state bits
#define PORT_STATE_ENABLED 0x01
#define PORT_STATE_CYCLE 0x02
#define PORT_STATE_FAULT 0x04

// Port fault cause bits
#define PORT_FAULT_OVERCURRENT 0x01
#define PORT_FAULT_OVERTEMP 0x02
#define PORT_FAULT_LIMIT 0x04 // in current limiting longer than the tolerance (D104)
#define PORT_FAULT_BUDGET 0x08 // switched off to bring the total current within the supply (D105)
#define PORT_FAULT_GENERIC 0x80

// Port warning bits
#define PORT_WARN_DISCHARGE 0x01
#define PORT_WARN_BACK_VOLTAGE 0x02
#define PORT_WARN_VBUS_LOW 0x04
#define PORT_WARN_BUDGET 0x08 // limit lowered to bring the total current within the supply (D105)

// Relay state bits
#define RELAY_STATE_DISCONNECTED 0x01
#define RELAY_STATE_PULSE 0x02

// System flags
#define SYS_INPUT_FAULT 0x01
#define SYS_HUB_CONFIGURED 0x02
#define SYS_CTRL12_ERROR 0x04
#define SYS_CTRL34_ERROR 0x08

// Capabilities
#define CAP_CDC_CONSOLE 0x00000001u

// Values in GET_STATUS when the reading fails
#define PROTO_TEMP_UNAVAILABLE (-128)
#define PROTO_CURRENT_UNAVAILABLE 0xFFFFu
#define PROTO_CHARGE_UNAVAILABLE 0xFFFFFFFFu

// Timing (to be validated on the prototype)
#define PORT_STAGGER_MS 100
#define PORT_CYCLE_DEFAULT_MS 1000
#define PORT_CYCLE_MIN_MS 500
#define PORT_CYCLE_MAX_MS 60000
#define RELAY_PULSE_MIN_MS 10
#define RELAY_PULSE_MAX_MS 60000

#endif
