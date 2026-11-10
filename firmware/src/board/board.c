#include "board/board.h"

#include <string.h>

#include "stm32g0xx_hal.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} pin_t;

#define PIN(p, n) {GPIO##p, GPIO_PIN_##n}

static const pin_t PIN_EN[BOARD_N_PORTS] = {PIN(A, 0), PIN(A, 1), PIN(C, 6), PIN(C, 7)};
static const pin_t PIN_OE_N[BOARD_N_PORTS] = {PIN(B, 3), PIN(B, 4), PIN(B, 14), PIN(D, 1)};
static const pin_t PIN_FAULT[BOARD_N_PORTS] = {PIN(B, 0), PIN(D, 3), PIN(B, 5), PIN(A, 4)};
static const pin_t PIN_LED[BOARD_N_PORTS] = {PIN(A, 5), PIN(A, 7), PIN(B, 15), PIN(A, 8)};
static const pin_t PIN_BTN[BOARD_N_BUTTONS] = {PIN(C, 13), PIN(C, 14), PIN(C, 15), PIN(B, 12), PIN(B, 13)};
static const pin_t PIN_LED_HOST = PIN(F, 0);
static const pin_t PIN_HUB_RST_N = PIN(F, 1);
static const pin_t PIN_HUB_CFG_SEL2 = PIN(A, 15);
static const pin_t PIN_RLY1 = PIN(D, 2);
static const pin_t PIN_EFUSE_FLT_N = PIN(B, 1);
static const pin_t PIN_I2C1_SCL = PIN(B, 8);

#define VBUS_DET_ADC_CHANNEL 10 // PB2
#define VBUS_DET_DIVIDER 2

static volatile uint8_t alert_edges;
static uint32_t reset_flags;

static void pin_write(pin_t p, bool high) {
    HAL_GPIO_WritePin(p.port, p.pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static bool pin_read(pin_t p) {
    return HAL_GPIO_ReadPin(p.port, p.pin) == GPIO_PIN_SET;
}

static bool pin_out_state(pin_t p) {
    return (p.port->ODR & p.pin) != 0;
}

static void pin_mode(pin_t p, uint32_t mode, uint32_t pull) {
    GPIO_InitTypeDef init = {.Pin = p.pin, .Mode = mode, .Pull = pull, .Speed = GPIO_SPEED_FREQ_LOW};
    HAL_GPIO_Init(p.port, &init);
}

// Preload the output latch, then switch the pin to output: no glitch.
static void pin_output(pin_t p, bool initial, uint32_t mode) {
    pin_write(p, initial);
    pin_mode(p, mode, GPIO_NOPULL);
}

static void clock_init(void) {
    // System clock stays on HSI16 (reset default). USB runs from HSI48,
    // trimmed on the USB start-of-frame by the CRS.
    RCC->CR &= ~RCC_CR_HSIDIV; // HSISYS = 16 MHz
    SystemCoreClockUpdate();

    RCC->CR |= RCC_CR_HSI48ON;
    while (!(RCC->CR & RCC_CR_HSI48RDY)) {
    }
    RCC->CCIPR2 &= ~RCC_CCIPR2_USBSEL; // 00 = HSI48

    __HAL_RCC_CRS_CLK_ENABLE();
    CRS->CFGR = (CRS->CFGR & ~CRS_CFGR_SYNCSRC) | CRS_CFGR_SYNCSRC_1; // sync on USB SOF
    CRS->CR |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;
}

static void adc_init(void) {
    __HAL_RCC_ADC_CLK_ENABLE();
    ADC1->CR = ADC_CR_ADVREGEN;
    HAL_Delay(1);
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {
    }
    ADC1->SMPR = 7; // 160.5 cycles: the divider is 100 kΩ
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {
    }
}

void board_init(void) {
    reset_flags = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;

    HAL_Init();
    clock_init();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // Remove the UCPD dead-battery pull-downs (Rd) from PA8, PB15, PD0, PD2
    // before those pins are driven (RM0444 SYSCFG_CFGR1, docs/pinout-mcu.md).
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_UCPD1_STROBE | SYSCFG_CFGR1_UCPD2_STROBE;

    // Power outputs first: ports off, data open, relay released.
    for (uint8_t i = 0; i < BOARD_N_PORTS; i++) {
        pin_output(PIN_EN[i], false, GPIO_MODE_OUTPUT_PP);
        pin_output(PIN_OE_N[i], true, GPIO_MODE_OUTPUT_PP);
    }
    pin_output(PIN_RLY1, false, GPIO_MODE_OUTPUT_PP);

    for (uint8_t i = 0; i < BOARD_N_PORTS; i++) {
        pin_output(PIN_LED[i], false, GPIO_MODE_OUTPUT_PP);
    }
    pin_output(PIN_LED_HOST, false, GPIO_MODE_OUTPUT_PP);

    // Hub control: reset line released (open-drain, never push-pull), CFG_SEL2
    // left to its pull-up until hub_ctrl decides.
    pin_output(PIN_HUB_RST_N, true, GPIO_MODE_OUTPUT_OD);
    pin_mode(PIN_HUB_CFG_SEL2, GPIO_MODE_INPUT, GPIO_NOPULL);

    for (uint8_t i = 0; i < BOARD_N_BUTTONS; i++) {
        pin_mode(PIN_BTN[i], GPIO_MODE_INPUT, GPIO_PULLUP);
    }

    // Fault inputs have external pull-ups. PA4 may carry a pull-down left by
    // the ROM bootloader: configuring it with no pull removes it.
    for (uint8_t i = 0; i < BOARD_N_PORTS; i++) {
        pin_mode(PIN_FAULT[i], GPIO_MODE_IT_FALLING, GPIO_NOPULL);
    }
    pin_mode(PIN_EFUSE_FLT_N, GPIO_MODE_INPUT, GPIO_NOPULL);

    // VBUS_DET: analog input on PB2.
    pin_mode((pin_t)PIN(B, 2), GPIO_MODE_ANALOG, GPIO_NOPULL);
    adc_init();

    EXTI->FPR1 = 0xFFFF;
    alert_edges = 0;
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI2_3_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

uint32_t board_millis(void) {
    return HAL_GetTick();
}

void board_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

void board_port_power(uint8_t port, bool on) {
    pin_write(PIN_EN[port], on);
}

void board_port_data(uint8_t port, bool on) {
    pin_write(PIN_OE_N[port], !on);
}

bool board_port_power_get(uint8_t port) {
    return pin_out_state(PIN_EN[port]);
}

bool board_port_data_get(uint8_t port) {
    return !pin_out_state(PIN_OE_N[port]);
}

bool board_port_alert(uint8_t port) {
    return !pin_read(PIN_FAULT[port]);
}

uint8_t board_port_alert_edges(void) {
    __disable_irq();
    uint8_t e = alert_edges;
    alert_edges = 0;
    __enable_irq();
    return e;
}

void board_led_port(uint8_t port, bool on) {
    pin_write(PIN_LED[port], on);
}

void board_led_host(bool on) {
    pin_write(PIN_LED_HOST, on);
}

void board_relay(bool energized) {
    pin_write(PIN_RLY1, energized);
}

bool board_relay_get(void) {
    return pin_out_state(PIN_RLY1);
}

bool board_button(uint8_t idx) {
    return !pin_read(PIN_BTN[idx]);
}

bool board_efuse_fault(void) {
    return !pin_read(PIN_EFUSE_FLT_N);
}

void board_hub_reset_drive(bool low) {
    pin_write(PIN_HUB_RST_N, !low);
}

bool board_hub_reset_level(void) {
    return pin_read(PIN_HUB_RST_N);
}

void board_hub_cfg_sel2(bool high) {
    pin_output(PIN_HUB_CFG_SEL2, high, GPIO_MODE_OUTPUT_PP);
}

bool board_i2c1_scl_level(void) {
    return pin_read(PIN_I2C1_SCL);
}

uint16_t board_vbus_det_mv(void) {
    ADC1->CHSELR = 1u << VBUS_DET_ADC_CHANNEL;
    ADC1->ISR = ADC_ISR_EOC;
    ADC1->CR |= ADC_CR_ADSTART;
    uint32_t t0 = HAL_GetTick();
    while (!(ADC1->ISR & ADC_ISR_EOC)) {
        if (HAL_GetTick() - t0 > 5) {
            return 0;
        }
    }
    uint32_t raw = ADC1->DR; // 12 bit, VREF+ = 3.3 V
    return (uint16_t)(raw * 3300u * VBUS_DET_DIVIDER / 4095u);
}

void board_uid(uint8_t uid[12]) {
    memcpy(uid, (const void *)UID_BASE, 12);
}

bool board_reset_was_power_on(void) {
    return (reset_flags & RCC_CSR_PWRRSTF) != 0;
}

const char *board_reset_cause(void) {
    if (reset_flags & RCC_CSR_PWRRSTF) {
        return "power-on";
    }
    if (reset_flags & RCC_CSR_IWDGRSTF) {
        return "watchdog";
    }
    if (reset_flags & RCC_CSR_SFTRSTF) {
        return "software";
    }
    if (reset_flags & RCC_CSR_OBLRSTF) {
        return "option-bytes";
    }
    if (reset_flags & RCC_CSR_PINRSTF) {
        return "nrst-pin";
    }
    return "unknown";
}

void board_watchdog_start(void) {
    // Freeze the watchdog while the core is halted by a debugger.
    __HAL_RCC_DBGMCU_CLK_ENABLE();
    DBG->APBFZ1 |= DBG_APB_FZ1_DBG_IWDG_STOP;

    IWDG->KR = 0xCCCC; // start (enables LSI)
    IWDG->KR = 0x5555; // unlock PR/RLR
    IWDG->PR = 4;      // LSI 32 kHz / 64 = 500 Hz
    IWDG->RLR = 1000;  // 2 s
    while (IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) {
    }
    IWDG->KR = 0xAAAA;
}

void board_watchdog_kick(void) {
    IWDG->KR = 0xAAAA;
}

// --- Interrupt handlers -------------------------------------------------------

void SysTick_Handler(void) {
    HAL_IncTick();
}

static void exti_dispatch(void) {
    uint32_t pending = EXTI->FPR1;
    EXTI->FPR1 = pending;
    for (uint8_t i = 0; i < BOARD_N_PORTS; i++) {
        if (pending & PIN_FAULT[i].pin) {
            alert_edges |= (uint8_t)(1u << i);
        }
    }
}

void EXTI0_1_IRQHandler(void) {
    exti_dispatch();
}

void EXTI2_3_IRQHandler(void) {
    exti_dispatch();
}

void EXTI4_15_IRQHandler(void) {
    exti_dispatch();
}

void HardFault_Handler(void) {
    // Leave the board in its safe state and let the watchdog restart it.
    for (uint8_t i = 0; i < BOARD_N_PORTS; i++) {
        PIN_EN[i].port->BRR = PIN_EN[i].pin;
    }
    for (;;) {
    }
}
