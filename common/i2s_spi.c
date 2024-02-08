#include "i2s_spi.h"

#define WS_PIN GPIO3

void i2s_spi_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_TIM3);

    // Configure Word Select (WS) pin as output
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, WS_PIN);

    /* Configure GPIOs: SS=PA4, SCK=PA5, MISO=PA6 and MOSI=PA7 */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4 | GPIO5 | GPIO7);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);

    /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
    rcc_periph_reset_pulse(RST_SPI1);

    /* Set up SPI in Master mode with:
     * Clock baud rate: 1/8 of peripheral clock frequency ~ 9MHz
     * Clock polarity: Idle High
     * Clock phase: Data valid on 2nd clock pulse
     * Data frame format: 16-bit
     * Frame format: MSB First
     */
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2,
                    SPI_CR1_DFF_16BIT, SPI_CR1_MSBFIRST);

    /*
     * Set NSS management to software.
     *
     * Note:
     * Setting nss high is very important, even if we are controlling the GPIO
     * ourselves this bit needs to be at least set to 1, otherwise the spi
     * peripheral will not send any data out.
     */
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);

    /* Enable SPI1 periph. */
    spi_enable(SPI1);

    // Timer3 Configuration
    rcc_periph_reset_pulse(RST_TIM3);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    // Our MCU is running at 72MHz, so we need to divide it by 72 to get 1MHz
    timer_set_prescaler(TIM3, 72 - 1);
    // 20 micros = 1MHz/20 = 50kHz ~ 48kHz
    timer_set_period(TIM3, 20 - 1);
    timer_set_oc_polarity_high(TIM3, TIM_OC1);
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_FROZEN);
    timer_enable_irq(TIM3, TIM_DIER_CC1IE);
    timer_enable_counter(TIM3);

    // Enable the NVIC interrupt for TIM3
    nvic_enable_irq(NVIC_TIM3_IRQ);
}

void i2s_send(uint16_t sample) {
    gpio_clear(GPIOA, WS_PIN);
    // spi_write(SPI1, sample);
    spi_send(SPI1, sample);

    // 2 or 3 microseconds
    // 1 is too few
    // delay_us(3);

    gpio_set(GPIOA, WS_PIN);
    // spi_write(SPI1, sample);
    spi_send(SPI1, sample);
}
