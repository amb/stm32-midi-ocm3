#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <stdlib.h>
#include <string.h>

#include "endless_encoder.h"
#include "ssd1306_128x32.h"
#include "tools.h"
#include "udelay.h"
#include "i2s_spi.h"

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

static uint16_t read_adc_naiive(uint8_t channel) {
    uint8_t channel_array[16];
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_start_conversion_direct(ADC1);
    while(!adc_eoc(ADC1)) {}
    return adc_read_regular(ADC1);
}

uint32_t phase[4] = {0};
int32_t knob = 0;

/* compute exp2(a) in s15.16 fixed-point arithmetic, -16 < a < 15 */
static int32_t fixed_exp2 (int32_t a)
{
    int32_t i, f, r, s;
    /* split a = i + f, such that f in [-0.5, 0.5] */
    i = (a + 0x8000) & ~0xffff; // 0.5
    f = a - i;   
    s = ((15 << 16) - i) >> 16;
    /* minimax approximation for exp2(f) on [-0.5, 0.5] */
    r = 0x00000e20;                 // 5.5171669058037949e-2
    r = (r * f + 0x3e1cc333) >> 17; // 2.4261112219321804e-1
    r = (r * f + 0x58bd46a6) >> 16; // 6.9326098546062365e-1
    r = r * f + 0x7ffde4a3;         // 9.9992807353939517e-1
    return (uint32_t)r >> s;
}

static void update_sample(void) {
    int16_t sample = 0;
    int16_t note = 0;
    int32_t freq = 0;

    // note = (knob / (8192/24));
    note = -12;
    freq = knob << 4;

    phase[0] += 440 * fixed_exp2(((note<<16)+freq)/12);
    phase[1] += 440 * fixed_exp2((((note+4)<<16)+freq)/12);
    phase[2] += 440 * fixed_exp2((((note+7)<<16)+freq)/12);

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
    sample =  ((int16_t)(phase[0] >> 16) - 32768) / 32;
    sample += ((int16_t)(phase[1] >> 16) - 32768) / 32;
    sample += ((int16_t)(phase[2] >> 16) - 32768) / 32;

    i2s_send(sample);
}

// I2S SPI uses TIM3 and SPI1
void tim3_isr() {
    if(timer_get_flag(TIM3, TIM_SR_CC1IF)) {
        timer_clear_flag(TIM3, TIM_SR_CC1IF);
        update_sample();
    }
}

int main(void) {
    int i;
    struct SSD1306 ssd1306;

    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    // LED
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_toggle(GPIOC, GPIO13);

    SSD1306_i2c_setup();
    adc_setup();
    delay_setup();
    i2s_spi_setup();

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

        if(encoder_update(&pot, adc1, adc2)) { screen_saver = 0; }

        SSD1306_clear(&ssd1306, 0x00);

        if(screen_saver < 30 * 20) {
            SSD1306_draw_string(&ssd1306, 0, 0, "ADC1:");
            SSD1306_print_number(&ssd1306, 8 * 5, 0, pot.smooth1);

            SSD1306_draw_string(&ssd1306, 0, 8, "ADC2:");
            SSD1306_print_number(&ssd1306, 8 * 5, 8, pot.smooth2);

            SSD1306_draw_string(&ssd1306, 0, 16, "totl:");
            SSD1306_print_number(&ssd1306, 8 * 5, 16, pot.total_value);

            SSD1306_draw_string(&ssd1306, 0, 24, "rato:");
            SSD1306_print_number(&ssd1306, 8 * 5, 24, pot.base_count);

            int px = 90 + (adc1 / 220);
            int py = (adc2 / 220);
            SSD1306_draw_pixel(&ssd1306, px, py);
            SSD1306_draw_pixel(&ssd1306, px + 1, py + 1);
            SSD1306_draw_pixel(&ssd1306, px + 1, py);
            SSD1306_draw_pixel(&ssd1306, px, py + 1);

            screen_saver++;

            knob = pot.total_value;
        }

        SSD1306_refresh(&ssd1306);

        // 30fps
        delay_us(1000000 / 30);
    }

    // free(ssd1306.screen_data);
    return 0;
}
