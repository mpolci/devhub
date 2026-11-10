#include "app/buttons.h"

#include "board/board.h"

#define DEBOUNCE_MS 20
// Minimum time between two accepted presses of the same button: contact
// chatter longer than the debounce time was seen toggling a port twice.
#define REPEAT_LOCKOUT_MS 250

typedef struct {
    bool stable;
    bool last_raw;
    uint32_t changed_at;
    uint32_t pressed_at;
    bool pressed_once;
    uint16_t count;
} button_t;

static button_t btn[BOARD_N_BUTTONS];
static button_cb_t callback;

void buttons_init(button_cb_t on_press) {
    callback = on_press;
    uint32_t now = board_millis();
    for (uint8_t i = 0; i < BOARD_N_BUTTONS; i++) {
        // A button held at start-up does not generate a press.
        bool raw = board_button(i);
        btn[i] = (button_t){.stable = raw, .last_raw = raw, .changed_at = now};
    }
}

void buttons_task(uint32_t now) {
    for (uint8_t i = 0; i < BOARD_N_BUTTONS; i++) {
        button_t *b = &btn[i];
        bool raw = board_button(i);
        if (raw != b->last_raw) {
            b->last_raw = raw;
            b->changed_at = now;
        } else if (raw != b->stable && (uint32_t)(now - b->changed_at) >= DEBOUNCE_MS) {
            b->stable = raw;
            if (raw && !(b->pressed_once && (uint32_t)(now - b->pressed_at) < REPEAT_LOCKOUT_MS)) {
                b->pressed_once = true;
                b->pressed_at = now;
                b->count++;
                if (callback) {
                    callback(i);
                }
            }
        }
    }
}

bool buttons_pressed(uint8_t idx) {
    return idx < BOARD_N_BUTTONS && btn[idx].stable;
}

uint16_t buttons_count(uint8_t idx) {
    return idx < BOARD_N_BUTTONS ? btn[idx].count : 0;
}
