#include "app/system.h"

#include "app/buttons.h"
#include "app/defaults.h"
#include "app/hub_ctrl.h"
#include "app/portctl.h"
#include "app/ports.h"
#include "app/relay.h"
#include "board/board.h"
#include "console/console.h"
#include "proto/proto.h"
#include "stm32g0xx_hal.h"

#define SYSTEM_MEMORY_BASE 0x1FFF0000u
#define BOOT_MAGIC 0xB007104Du
// Delay before a requested reset, so that the HID response can leave.
#define REQUEST_DELAY_MS 100

typedef enum { REQ_NONE, REQ_REBOOT, REQ_BOOTLOADER, REQ_HUB_RESET } request_t;

static uint32_t boot_magic __attribute__((section(".noinit")));

static request_t request;
static uint32_t request_at;
static uint8_t last_flags;

// True when the core and clock tree are in their reset state. A start through
// the ROM bootloader's "leave" jump is not: it keeps the bootloader's vector
// table, interrupts, SysTick and clocks.
static bool started_from_reset(void) {
    return SCB->VTOR == 0 && NVIC->ISER[0] == 0 && !(SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) &&
           (RCC->CFGR & RCC_CFGR_SWS) == 0;
}

void system_early(void) {
    // The "flash empty" flag is only re-evaluated at power-on: after the first
    // programming it would send every later reset back to the bootloader.
    FLASH->ACR &= ~FLASH_ACR_PROGEMPTY;

    if (!started_from_reset()) {
        NVIC_SystemReset(); // restart cleanly from the main flash
    }
    if (boot_magic != BOOT_MAGIC) {
        return;
    }
    boot_magic = 0;
    // Fresh out of reset: clocks and peripherals are in their default state,
    // as the ROM bootloader expects. The hub is not touched.
    const uint32_t *vectors = (const uint32_t *)SYSTEM_MEMORY_BASE;
    __set_MSP(vectors[0]);
    ((void (*)(void))vectors[1])();
    for (;;) {
    }
}

// --- Relay and buttons ---------------------------------------------------------

static void relay_on_state(uint8_t state, uint8_t origin) {
    LOG("relay: state 0x%02X (origin %u)", state, origin);
    proto_event(EVT_RELAY_CHANGED, (const uint8_t[]){1, state, origin}, 3);
}

static const relay_hw_t relay_hw = {.set_coil = board_relay, .on_state = relay_on_state};

static void on_button(uint8_t idx) {
    if (idx == BOARD_BUTTON_RELAY) {
        relay_button();
    } else {
        ports_button(idx, board_millis());
    }
}

// --- Protocol services ---------------------------------------------------------

static const proto_sys_t proto_sys = {
    .now = board_millis,
    .system_flags = system_flags,
    .mcu_temp_c = system_mcu_temp_c,
    .get_uid = board_uid,
    .port_measure = portctl_measure,
    .port_reset_charge = portctl_reset_charge,
    .defaults_get = defaults_get,
    .defaults_set = defaults_set,
    .defaults_save = defaults_save,
    .charge_reset_get = defaults_charge_reset_get,
    .charge_reset_set = defaults_charge_reset_set,
    .port_limit_get = defaults_port_limit_get,
    .port_limit_set = portctl_set_limit,
    .supply_get = defaults_supply_get,
    .supply_set = defaults_supply_set,
    .label_get = defaults_label_get,
    .label_set = defaults_label_set,
    .request_hub_reset = system_request_hub_reset,
    .request_reboot = system_request_reboot,
    .request_bootloader = system_request_bootloader,
};

static void schedule(request_t r) {
    request = r;
    request_at = board_millis() + REQUEST_DELAY_MS;
}

void system_request_reboot(void) {
    schedule(REQ_REBOOT);
}

void system_request_bootloader(void) {
    schedule(REQ_BOOTLOADER);
}

void system_request_hub_reset(void) {
    schedule(REQ_HUB_RESET);
}

int8_t system_mcu_temp_c(void) {
    int16_t t;
    if (!board_mcu_temp_c(&t)) {
        return PROTO_TEMP_UNAVAILABLE;
    }
    return (int8_t)(t < -127 ? -127 : t > 127 ? 127 : t);
}

uint8_t system_flags(void) {
    return (uint8_t)((board_efuse_fault() ? SYS_INPUT_FAULT : 0) | (hub_ctrl_configured() ? SYS_HUB_CONFIGURED : 0) |
                     (portctl_chip_ok(0) ? 0 : SYS_CTRL12_ERROR) | (portctl_chip_ok(1) ? 0 : SYS_CTRL34_ERROR));
}

void system_init(void) {
    proto_init(&proto_sys);
    defaults_init();
    relay_init(&relay_hw);
    buttons_init(on_button);
    last_flags = system_flags();
}

void system_task(uint32_t now) {
    relay_task(now);

    uint8_t flags = system_flags();
    if (flags != last_flags) {
        last_flags = flags;
        LOG("system: flags 0x%02X", flags);
        proto_event(EVT_POWER_CHANGED, &flags, 1);
    }

    if (request != REQ_NONE && (int32_t)(now - request_at) >= 0) {
        request_t r = request;
        request = REQ_NONE;
        switch (r) {
        case REQ_HUB_RESET:
            hub_ctrl_request_reset();
            break;
        case REQ_BOOTLOADER:
            boot_magic = BOOT_MAGIC;
            NVIC_SystemReset();
            break;
        case REQ_REBOOT:
            NVIC_SystemReset();
            break;
        default:
            break;
        }
    }
}
