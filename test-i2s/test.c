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

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>

#define WS_PIN GPIO3
// #define LFO_POT_1 GPIO1
// #define LFO_POT_2 GPIO2

uint16_t phase[4] = {0};
uint16_t sample = 0;

void update_sample(void);

static void delay_setup(void) {
    rcc_periph_clock_enable(RCC_TIM2);
    timer_set_prescaler(TIM2, rcc_apb1_frequency / 1000000 - 1);
    timer_set_period(TIM2, 0xffff);
    timer_one_shot_mode(TIM2);
}

static void delay_us(uint32_t us) {
    TIM_ARR(TIM2) = us;
    TIM_EGR(TIM2) = TIM_EGR_UG;
    TIM_CR1(TIM2) |= TIM_CR1_CEN;
    while(TIM_CR1(TIM2) & TIM_CR1_CEN) {}
}

static void setup(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_TIM3);

    // Configure Word Select (WS) pin as output
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, WS_PIN);

    /* Configure GPIOs: SS=PA4, SCK=PA5, MISO=PA6 and MOSI=PA7 */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4 | GPIO5 | GPIO7);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);

    /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
    rcc_periph_reset_pulse(RST_SPI1);

    /* Set up SPI in Master mode with:
     * Clock baud rate: 1/8 of peripheral clock frequency
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
    // spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);

    /* Enable SPI1 periph. */
    spi_enable(SPI1);

    // Timer3 Configuration
    rcc_periph_reset_pulse(RST_TIM3);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM3, 72 - 1);
    timer_set_period(TIM3, 20 - 1);
    timer_set_oc_polarity_high(TIM3, TIM_OC1);
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_FROZEN);
    timer_enable_irq(TIM3, TIM_DIER_CC1IE);
    timer_enable_counter(TIM3);

    // Enable the NVIC interrupt for TIM3
    nvic_enable_irq(NVIC_TIM3_IRQ);

    delay_setup();
}

void tim3_isr() {
    if(timer_get_flag(TIM3, TIM_SR_CC1IF)) {
        timer_clear_flag(TIM3, TIM_SR_CC1IF);
        update_sample();
    }
}

void update_sample() {
    phase[0] += 220;
    phase[1] += 549;
    phase[2] += 661;

    // Square
    // sample =  ((phase[0] < 32768) * 65635) / 4;
    // sample += ((phase[1] < 32768) * 65635) / 4;
    // sample += ((phase[2] < 32768) * 65635) / 4;

    // Triangle
    // if(phase[0] < 32768) {
    //     sample = phase[0];
    // } else {
    //     sample = 32768 - (phase[0]-32768);
    // }

    // Saw
    sample = phase[0];

    // Your sample update logic here
    gpio_clear(GPIOA, WS_PIN);
    spi_write(SPI1, sample);

    // 2 or 3 microseconds
    // 1 is too few
    delay_us(2);

    gpio_set(GPIOA, WS_PIN);
    spi_write(SPI1, sample);
}

int main(void) {
    setup();

    while(1) {
        // Main loop
    }

    return 0;
}
