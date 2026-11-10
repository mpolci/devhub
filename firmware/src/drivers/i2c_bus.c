#include "drivers/i2c_bus.h"

#include "stm32g0xx_hal.h"

#define I2C_TIMEOUT_MS 20
// 100 kHz with a 16 MHz kernel clock (PCLK), rise 100 ns / fall 10 ns.
#define I2C_TIMING_100K_16MHZ 0x00303D5Bu

static I2C_HandleTypeDef handles[I2C_BUS_COUNT];
static bool ready[I2C_BUS_COUNT];

static void pins(i2c_bus_t bus, bool i2c_mode) {
    GPIO_InitTypeDef init = {
        .Pin = (bus == I2C_BUS_1) ? (GPIO_PIN_8 | GPIO_PIN_9) : (GPIO_PIN_10 | GPIO_PIN_11),
        .Mode = i2c_mode ? GPIO_MODE_AF_OD : GPIO_MODE_INPUT,
        .Pull = GPIO_NOPULL, // external 4.7 kΩ pull-ups
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF6_I2C1, // AF6 for both I2C1 and I2C2 on port B
    };
    HAL_GPIO_Init(GPIOB, &init);
}

// A slave interrupted in the middle of a read can hold SDA low: clock it out.
static void recover(i2c_bus_t bus) {
    uint16_t scl = (bus == I2C_BUS_1) ? GPIO_PIN_8 : GPIO_PIN_10;
    uint16_t sda = (bus == I2C_BUS_1) ? GPIO_PIN_9 : GPIO_PIN_11;
    GPIO_InitTypeDef init = {.Pin = sda, .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL};
    HAL_GPIO_Init(GPIOB, &init);
    if (HAL_GPIO_ReadPin(GPIOB, sda) == GPIO_PIN_SET) {
        return;
    }
    HAL_GPIO_WritePin(GPIOB, scl, GPIO_PIN_SET);
    init.Pin = scl;
    init.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(GPIOB, &init);
    for (uint8_t i = 0; i < 9 && HAL_GPIO_ReadPin(GPIOB, sda) == GPIO_PIN_RESET; i++) {
        HAL_GPIO_WritePin(GPIOB, scl, GPIO_PIN_RESET);
        for (volatile uint32_t d = 0; d < 40; d++) {
        }
        HAL_GPIO_WritePin(GPIOB, scl, GPIO_PIN_SET);
        for (volatile uint32_t d = 0; d < 40; d++) {
        }
    }
}

void i2c_bus_init(i2c_bus_t bus) {
    I2C_HandleTypeDef *h = &handles[bus];
    recover(bus);
    if (bus == I2C_BUS_1) {
        __HAL_RCC_I2C1_CLK_ENABLE();
        __HAL_RCC_I2C1_FORCE_RESET();
        __HAL_RCC_I2C1_RELEASE_RESET();
        h->Instance = I2C1;
    } else {
        __HAL_RCC_I2C2_CLK_ENABLE();
        __HAL_RCC_I2C2_FORCE_RESET();
        __HAL_RCC_I2C2_RELEASE_RESET();
        h->Instance = I2C2;
    }
    pins(bus, true);
    h->State = HAL_I2C_STATE_RESET;
    h->Init.Timing = I2C_TIMING_100K_16MHZ;
    h->Init.OwnAddress1 = 0;
    h->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    h->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    h->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    h->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    ready[bus] = HAL_I2C_Init(h) == HAL_OK;
}

void i2c_bus_deinit(i2c_bus_t bus) {
    if (ready[bus]) {
        HAL_I2C_DeInit(&handles[bus]);
    }
    pins(bus, false);
    ready[bus] = false;
}

bool i2c_bus_ready(i2c_bus_t bus) {
    return ready[bus];
}

// A failed transfer can leave the peripheral busy: re-initialise it.
static bool done(i2c_bus_t bus, HAL_StatusTypeDef st) {
    if (st == HAL_OK) {
        return true;
    }
    i2c_bus_init(bus);
    return false;
}

bool i2c_write(i2c_bus_t bus, uint8_t addr7, const uint8_t *data, uint16_t len) {
    if (!ready[bus]) {
        return false;
    }
    return done(bus, HAL_I2C_Master_Transmit(&handles[bus], (uint16_t)(addr7 << 1), (uint8_t *)data, len,
                                             I2C_TIMEOUT_MS));
}

bool i2c_read_reg(i2c_bus_t bus, uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len) {
    if (!ready[bus]) {
        return false;
    }
    return done(bus, HAL_I2C_Mem_Read(&handles[bus], (uint16_t)(addr7 << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len,
                                      I2C_TIMEOUT_MS));
}

bool i2c_write_reg(i2c_bus_t bus, uint8_t addr7, uint8_t reg, uint8_t value) {
    if (!ready[bus]) {
        return false;
    }
    return done(bus, HAL_I2C_Mem_Write(&handles[bus], (uint16_t)(addr7 << 1), reg, I2C_MEMADD_SIZE_8BIT, &value, 1,
                                       I2C_TIMEOUT_MS));
}

bool i2c_probe(i2c_bus_t bus, uint8_t addr7) {
    if (!ready[bus]) {
        return false;
    }
    return done(bus, HAL_I2C_IsDeviceReady(&handles[bus], (uint16_t)(addr7 << 1), 1, I2C_TIMEOUT_MS));
}
