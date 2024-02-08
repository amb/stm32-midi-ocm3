#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <stdlib.h>
#include <string.h>

#include "endless_encoder.h"
#include "i2s_spi.h"
#include "midi.h"
#include "ssd1306_128x32.h"
#include "tools.h"
#include "udelay.h"

usbd_device *usbd_dev;
static const char *usb_strings[] = {"ambi.tech", "midifiddler", usb_serial_number};
uint32_t total_received = 0;

uint32_t phase[4] = {0};
uint16_t amplitude[4] = {0};
int16_t notes[4] = {0};

uint8_t note_pointer = 0;

int32_t knob = 0;

static void usbmidi_data_rx_cb(usbd_device *ubd, uint8_t ep) {
    (void)ep;

    char buf[64];
    int len = usbd_ep_read_packet(ubd, 0x01, buf, 64);

    // Note on
    if(buf[1] == 144) {
        notes[note_pointer] = buf[2] - 69;
        amplitude[note_pointer] = 65535;

        note_pointer++;
        if(note_pointer >= 3) { note_pointer = 0; }
    }

    total_received++;
}

static void usbmidi_set_config(usbd_device *ubd, uint16_t wValue) {
    (void)wValue;
    usbd_ep_setup(ubd, 0x01, USB_ENDPOINT_ATTR_BULK, 64, usbmidi_data_rx_cb);
    usbd_ep_setup(ubd, 0x81, USB_ENDPOINT_ATTR_BULK, 64, NULL);
}

// static void send_test_output(void) {
//     char buf[4] = {
//         0x08, /* USB framing: virtual cable 0, note on */
//         0x80, /* MIDI command: note on, channel 1 */
//         60,   /* Note 60 (middle C) */
//         64,   /* "Normal" velocity */
//     };
//     while(usbd_ep_write_packet(usbd_dev, 0x81, buf, sizeof(buf)) == 0) {};
// }

static void usb_midi_setup(void) {
    usbd_dev = usb_start(usb_strings);
    usbd_register_set_config_callback(usbd_dev, usbmidi_set_config);
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

static uint16_t read_adc_naiive(uint8_t channel) {
    uint8_t channel_array[16];
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_start_conversion_direct(ADC1);
    while(!adc_eoc(ADC1)) {}
    return adc_read_regular(ADC1);
}

static void update_sample(void) {
    int32_t sample = 0;
    int32_t freq = 0;

    for(int i = 0; i < 3; i++) {
        if(amplitude[i] > 0) { amplitude[i]--; }
    }

    // note = (knob / (8192/24));
    freq = knob << 4;

    phase[0] += 440 * fixed_exp2(((notes[0] << 16) + freq) / 12);
    phase[1] += 440 * fixed_exp2(((notes[1] << 16) + freq) / 12);
    phase[2] += 440 * fixed_exp2(((notes[2] << 16) + freq) / 12);

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
    sample = 0;
    for (int i = 0; i < 3; i++) {
        int32_t temp = (phase[i] / 65536) - 32768;
        temp /= 8;
        temp *= amplitude[i];
        temp /= 65536;
        sample += temp;
    }

    if (sample > 32000 || sample < -32000) {
        // Distortion alert
        gpio_toggle(GPIOC, GPIO13);
    }

    i2s_send(sample + 32768);
    // i2s_send((uint16_t)(sample + 32768));
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

    usb_midi_setup();

    EndlessEncoder pot = {0};

    uint16_t adc1 = 0;
    uint16_t adc2 = 0;

    uint16_t screen_saver = 0;

    for(i = 0; i < 400000; i++) { usbd_poll(usbd_dev); }

    while(true) {
        usbd_poll(usbd_dev);

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

            SSD1306_draw_string(&ssd1306, 0, 24, "MIDI:");
            SSD1306_print_number(&ssd1306, 8 * 5, 24, total_received);

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
        for(i = 0; i < 10; i++) {
            usbd_poll(usbd_dev);
            delay_us(1000000 / 300);
        }
    }

    // free(ssd1306.screen_data);
    return 0;
}
