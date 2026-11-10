// Debounced panel buttons, polled from the main loop.
#ifndef APP_BUTTONS_H
#define APP_BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*button_cb_t)(uint8_t idx);

void buttons_init(button_cb_t on_press);
void buttons_task(uint32_t now);
bool buttons_pressed(uint8_t idx);     // debounced state
uint16_t buttons_count(uint8_t idx);   // presses since start-up

#endif
