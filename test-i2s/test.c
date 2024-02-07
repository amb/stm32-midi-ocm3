
// ST AN5086
// I2S: Master transmitter I2S emulator with SPI and STM32 MCU
// SPI_MOSI -> I2S_SD
// SPI_SCK -> I2S_SCK
// TIMER_OUT -> I2S_WS
// TIMER_TRG <- SCK
// SYSCLK <- External clock

// blue pill SPI ports

// SPI1
// SPI1_MOSI -> PA7 (PB5 alt)
// SPI1_SCLK -> PA5 (PB3 alt)

// SPI2 (no alternative ports)
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

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#define WS_PIN GPIO3
// #define LFO_POT_1 GPIO1
// #define LFO_POT_2 GPIO2

void update_sample(void);

static void setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_TIM3);

    // // Configure pins for LFO Potentiometers
    // gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, LFO_POT_1 | LFO_POT_2);

    // Configure Word Select (WS) pin as output
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, WS_PIN);

    // SPI Configuration
    spi_set_master_mode(SPI1);
    spi_set_baudrate_prescaler(SPI1, SPI_CR1_BR_FPCLK_DIV_4);
    // spi_set_data_size(SPI1, SPI_CR2_DS_16BIT);
    SPI_CR2(SPI1) |= SPI_I2SCFGR_I2SSTD_MSB_JUSTIFIED;
    spi_set_clock_polarity_0(SPI1);
    spi_set_clock_phase_1(SPI1);
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);
    spi_enable(SPI1);

    // Timer3 Configuration
    rcc_periph_reset_pulse(RST_TIM3);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM3, 72 - 1);
    timer_set_period(TIM3, 20 - 1);
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_FROZEN);
    timer_enable_irq(TIM3, TIM_DIER_CC1IE);
    timer_enable_counter(TIM3);

    // Enable the NVIC interrupt for TIM3
    nvic_enable_irq(NVIC_TIM3_IRQ);
}

void tim3_isr() {
    if (timer_get_flag(TIM3, TIM_SR_CC1IF)) {
        timer_clear_flag(TIM3, TIM_SR_CC1IF);
        update_sample();
    }
}

void update_sample() {
    // Your sample update logic here
}

int main(void) {
    setup();

    while (1) {
        // Main loop
    }

    return 0;
}

