#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <stdlib.h>
#include <string.h>

#include "ssd1306_128x32.h"
#include "endless_encoder.h"
#include "tools.h"

static void i2c_setup(void) {
    /* Enable GPIOB and I2C1 clocks */
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_AFIO);

    /* Set alternate functions for SCL and SDA pins of I2C1 */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C1_SCL | GPIO_I2C1_SDA);

    /* Disable the I2C peripheral before configuration */
    i2c_peripheral_disable(I2C1);

    /* APB1 running at 36MHz */
    // i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);
    i2c_set_clock_frequency(I2C1, 36);

    /* 400kHz - I2C fast mode */
    i2c_set_fast_mode(I2C1);
    i2c_set_ccr(I2C1, 0x1e);
    i2c_set_trise(I2C1, 0x0b);

    /* And go */
    i2c_peripheral_enable(I2C1);
}

static void adc_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_ADC1);

    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);

    /* Make sure the ADC doesn't run during config. */
    adc_power_off(ADC1);

    /* We configure everything for one single conversion. */
    adc_disable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);

    adc_power_on(ADC1);

    /* Wait for ADC starting up. */
    int i;
    for(i = 0; i < 800000; i++) /* Wait a bit. */
        __asm__("nop");

    adc_reset_calibration(ADC1);
    adc_calibrate(ADC1);
}

static void delay_setup(void) {
    rcc_periph_clock_enable(RCC_TIM2);
    timer_set_prescaler(TIM2, rcc_apb1_frequency / 1000000 - 1);
    timer_set_period(TIM2, 0xffff);
    timer_one_shot_mode(TIM2);
}

static uint16_t read_adc_naiive(uint8_t channel) {
    uint8_t channel_array[16];
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_start_conversion_direct(ADC1);
    while(!adc_eoc(ADC1)) {}
    return adc_read_regular(ADC1);
}

static void delay_us(uint32_t us) {
    TIM_ARR(TIM2) = us;
    TIM_EGR(TIM2) = TIM_EGR_UG;
    TIM_CR1(TIM2) |= TIM_CR1_CEN;
    while(TIM_CR1(TIM2) & TIM_CR1_CEN) {}
}


int main(void) {
    int i;
    struct SSD1306 ssd1306;

    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_toggle(GPIOC, GPIO13);

    i2c_setup();
    adc_setup();
    delay_setup();

    SSD1306_init(&ssd1306, I2C1);

    SSD1306_clear(&ssd1306, 0x00);
    SSD1306_refresh(&ssd1306);

    EndlessEncoder pot = {0};

    uint16_t adc1 = 0;
    uint16_t adc2 = 0;

    uint16_t screen_saver = 0;

    while(true) {
        adc1 = read_adc_naiive(1);
        adc2 = read_adc_naiive(2);

        if(encoder_update_smooth(&pot, adc1, adc2)) {
            screen_saver = 0;
        }

        int32_t total_value = encoder_get_total(&pot);

        // SSD1306_clear(&ssd1306, adc1 & 0xFF);
        SSD1306_clear(&ssd1306, 0x00);

        if(screen_saver < 30 * 20) {
            SSD1306_draw_string(&ssd1306, 0, 0, "ADC1:");
            SSD1306_print_number(&ssd1306, 8 * 5, 0, pot.smooth1);

            SSD1306_draw_string(&ssd1306, 0, 8, "ADC2:");
            SSD1306_print_number(&ssd1306, 8 * 5, 8, pot.smooth2);

            SSD1306_draw_string(&ssd1306, 0, 16, "totl:");
            SSD1306_print_number(&ssd1306, 8 * 5, 16, total_value);

            SSD1306_draw_string(&ssd1306, 0, 24, "rato:");
            SSD1306_print_number(&ssd1306, 8 * 5, 24, pot.base_count);

            int px = 90 + (adc1 / 220);
            int py = (adc2 / 220);
            SSD1306_draw_pixel(&ssd1306, px, py);
            SSD1306_draw_pixel(&ssd1306, px + 1, py + 1);
            SSD1306_draw_pixel(&ssd1306, px + 1, py);
            SSD1306_draw_pixel(&ssd1306, px, py + 1);

            screen_saver++;
        }

        SSD1306_refresh(&ssd1306);

        // 30fps
        delay_us(1000000 / 30);
    }

    // free(ssd1306.screen_data);
    return 0;
}

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
