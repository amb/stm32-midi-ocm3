// SPI2_MOSI -> PB15
// SPI2_SCLK -> PB13

// System clock = HSI/MSI or HSE
// I2S serial data and clock = SPI (standard)
// I2S frame rate = TIMER

// Timer peripheral
// The polarity of the timer, the counter period and its initial values are set to match the I²S
// protocol configuration:
// • period value = 31
// • pulse value = 15
// • trigger polarity = rising
// • output comparator polarity = TIM_OCPOLARITY_HIGH

// Calculation of the I²S interface bit rate
// I2S BITRATE SAMPLE RATE WIDTH× CHANNEL× 48000 16 2×× 1.536 Mbit/s

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>

#include "udelay.h"

#define WS_PIN GPIO3

void i2s_spi_setup(void);

void i2s_send(uint16_t sample);
