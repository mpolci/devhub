#include <string.h>

#include "board/board.h"
#include "proto/proto_defs.h"
#include "tusb.h"
#include "usb/usb_dev.h"

enum { ITF_HID = 0, ITF_CDC, ITF_CDC_DATA, ITF_COUNT };

#define EP_HID_OUT 0x01
#define EP_HID_IN 0x81
#define EP_CDC_NOTIF 0x82
#define EP_CDC_OUT 0x03
#define EP_CDC_IN 0x83

enum { STR_LANG = 0, STR_MANUFACTURER, STR_PRODUCT, STR_SERIAL, STR_HID, STR_CDC };

#define REPORT_PAYLOAD (PROTO_REPORT_SIZE - 1)

// One vendor-defined report per direction and purpose, 63 payload bytes each.
#define VENDOR_REPORT(id, usage, item)                                                                           \
    0x85, (id), 0x09, (usage), 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, REPORT_PAYLOAD, (item), 0x02

static const uint8_t desc_hid_report[] = {
    0x06, 0x00, 0xFF, // Usage Page (Vendor 0xFF00)
    0x09, 0x01,       // Usage (1)
    0xA1, 0x01,       // Collection (Application)
    VENDOR_REPORT(REPORT_ID_COMMAND, 0x02, 0x91),  // Output: command
    VENDOR_REPORT(REPORT_ID_RESPONSE, 0x03, 0x81), // Input: response
    VENDOR_REPORT(REPORT_ID_EVENT, 0x04, 0x81),    // Input: event
    0xC0,
};

static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    // Composite device with an interface association (CDC).
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = (FW_VERSION_MAJOR << 8) | (FW_VERSION_MINOR << 4) | FW_VERSION_PATCH,
    .iManufacturer = STR_MANUFACTURER,
    .iProduct = STR_PRODUCT,
    .iSerialNumber = STR_SERIAL,
    .bNumConfigurations = 1,
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t desc_configuration[] = {
    // Self-powered: the MCU is supplied by the hub's own power input.
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_SELF_POWERED, 2),
    TUD_HID_INOUT_DESCRIPTOR(ITF_HID, STR_HID, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EP_HID_OUT, EP_HID_IN,
                             PROTO_REPORT_SIZE, 1),
    TUD_CDC_DESCRIPTOR(ITF_CDC, STR_CDC, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 64),
};

const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t *)&desc_device;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc[32];
    char serial[25];
    const char *str;
    uint8_t len = 0;

    switch (index) {
    case STR_LANG:
        desc[1] = 0x0409;
        len = 1;
        goto done;
    case STR_MANUFACTURER:
        str = "devhub";
        break;
    case STR_PRODUCT:
        str = "devhub control";
        break;
    case STR_SERIAL: {
        uint8_t uid[12];
        board_uid(uid);
        for (uint8_t i = 0; i < 12; i++) {
            serial[2 * i] = "0123456789ABCDEF"[uid[i] >> 4];
            serial[2 * i + 1] = "0123456789ABCDEF"[uid[i] & 0x0F];
        }
        serial[24] = 0;
        str = serial;
        break;
    }
    case STR_HID:
        str = "devhub control (HID)";
        break;
    case STR_CDC:
        str = "devhub console";
        break;
    default:
        return NULL;
    }
    len = (uint8_t)strlen(str);
    if (len > 31) {
        len = 31;
    }
    for (uint8_t i = 0; i < len; i++) {
        desc[1 + i] = (uint8_t)str[i];
    }
done:
    desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * len + 2));
    return desc;
}
