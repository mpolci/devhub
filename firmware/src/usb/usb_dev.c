#include "usb/usb_dev.h"

#include <string.h>

#include "board/board.h"
#include "proto/proto.h"
#include "stm32g0xx_hal.h"
#include "tusb.h"

void usb_dev_init(void) {
    HAL_PWREx_EnableVddUSB();
    __HAL_RCC_USB_CLK_ENABLE();

    tusb_rhport_init_t init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
    tusb_init(0, &init);
}

void usb_dev_task(void) {
    tud_task();

    // Drain the protocol queue: one report per free IN slot.
    const uint8_t *r = proto_tx_peek();
    if (r && tud_hid_ready()) {
        if (tud_hid_report(r[0], &r[1], PROTO_REPORT_SIZE - 1)) {
            proto_tx_pop();
        }
    }
    if (!tud_mounted()) {
        // Nobody is listening: do not let stale reports pile up.
        while (proto_tx_peek()) {
            proto_tx_pop();
        }
    }
    board_led_host(usb_dev_host_active());
}

bool usb_dev_host_active(void) {
    return tud_mounted() && !tud_suspended();
}

void usb_cdc_write(const char *data, uint16_t len) {
    if (!tud_cdc_connected()) {
        return;
    }
    tud_cdc_write(data, len); // bytes that do not fit are dropped
    tud_cdc_write_flush();
}

int usb_cdc_getc(void) {
    if (!tud_cdc_available()) {
        return -1;
    }
    return tud_cdc_read_char();
}

// --- TinyUSB callbacks ---------------------------------------------------------

uint32_t tusb_time_millis_api(void) {
    return board_millis();
}

void USB_UCPD1_2_IRQHandler(void) {
    tusb_int_handler(0, true);
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    return 0; // reports are only sent on the interrupt IN endpoint
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, const uint8_t *buffer,
                           uint16_t bufsize) {
    if (report_type != HID_REPORT_TYPE_OUTPUT) {
        return;
    }
    if (report_id == 0) {
        // From the interrupt OUT endpoint: the report ID is the first byte.
        proto_handle(buffer, bufsize);
    } else {
        // From a SET_REPORT control request: the ID was stripped.
        uint8_t report[PROTO_REPORT_SIZE] = {report_id};
        if (bufsize > PROTO_REPORT_SIZE - 1) {
            bufsize = PROTO_REPORT_SIZE - 1;
        }
        memcpy(&report[1], buffer, bufsize);
        proto_handle(report, (uint16_t)(bufsize + 1));
    }
}
