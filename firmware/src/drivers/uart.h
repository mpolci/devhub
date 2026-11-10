// Interrupt-driven console UART (USART2 on J9, 115200 8N1).
#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

#include <stdint.h>

void uart_init(void);
// Queue bytes for transmission; bytes that do not fit are dropped.
void uart_write(const char *data, uint16_t len);
// Returns the next received byte, or -1.
int uart_getc(void);

#endif
