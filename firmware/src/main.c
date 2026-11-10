// devhub firmware: cooperative main loop, 1 ms tick, no RTOS.
#include "app/buttons.h"
#include "app/hub_ctrl.h"
#include "app/portctl.h"
#include "app/system.h"
#include "board/board.h"
#include "console/console.h"
#include "usb/usb_dev.h"

int main(void) {
    system_early();

    board_init();
    console_init();
    LOG("devhub firmware %d.%d.%d, reset cause: %s", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH,
        board_reset_cause());

    system_init();
    hub_ctrl_init();
    usb_dev_init();
    portctl_init();
    board_watchdog_start();

    for (;;) {
        uint32_t now = board_millis();
        board_watchdog_kick();
        hub_ctrl_task(now);
        usb_dev_task();
        buttons_task(now);
        portctl_task(now);
        system_task(now);
        console_task();
    }
}
