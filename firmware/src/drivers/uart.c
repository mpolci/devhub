#include "drivers/uart.h"

#include "stm32g0xx_hal.h"

#define BAUD 115200u
#define TX_SIZE 2048u // power of two
#define RX_SIZE 128u  // power of two

static uint8_t tx_buf[TX_SIZE];
static volatile uint16_t tx_head, tx_tail;
static uint8_t rx_buf[RX_SIZE];
static volatile uint16_t rx_head, rx_tail;

void uart_init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    GPIO_InitTypeDef init = {
        .Pin = GPIO_PIN_2 | GPIO_PIN_3,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF1_USART2,
    };
    HAL_GPIO_Init(GPIOA, &init);

    USART2->CR1 = 0;
    USART2->BRR = (HAL_RCC_GetPCLK1Freq() + BAUD / 2) / BAUD;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE_RXFNEIE | USART_CR1_UE;

    HAL_NVIC_SetPriority(USART2_LPUART2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART2_LPUART2_IRQn);
}

void uart_write(const char *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (uint16_t)((tx_head + 1) & (TX_SIZE - 1));
        if (next == tx_tail) {
            break; // full: drop the rest
        }
        tx_buf[tx_head] = (uint8_t)data[i];
        tx_head = next;
    }
    USART2->CR1 |= USART_CR1_TXEIE_TXFNFIE;
}

int uart_getc(void) {
    if (rx_head == rx_tail) {
        return -1;
    }
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1) & (RX_SIZE - 1));
    return c;
}

void USART2_LPUART2_IRQHandler(void) {
    uint32_t isr = USART2->ISR;
    if (isr & USART_ISR_RXNE_RXFNE) {
        uint8_t c = (uint8_t)USART2->RDR;
        uint16_t next = (uint16_t)((rx_head + 1) & (RX_SIZE - 1));
        if (next != rx_tail) {
            rx_buf[rx_head] = c;
            rx_head = next;
        }
    }
    if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
    }
    if ((isr & USART_ISR_TXE_TXFNF) && (USART2->CR1 & USART_CR1_TXEIE_TXFNFIE)) {
        if (tx_head != tx_tail) {
            USART2->TDR = tx_buf[tx_tail];
            tx_tail = (uint16_t)((tx_tail + 1) & (TX_SIZE - 1));
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;
        }
    }
}
