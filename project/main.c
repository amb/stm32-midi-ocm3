#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <stdlib.h>
#include <string.h>

#include "font8x8_basic.h"

#define WIDTH 128
#define HEIGHT 32

// Hardware description
#define SSD1306_I2C_ADDRESS 0x3C  // default I2C address
#define SSD1306_PAGE_START 0
#define SSD1306_PAGE_STOP ((WIDTH / 8) - 1)
#define SSD1306_COL_START 0
#define SSD1306_COL_STOP (HEIGHT - 1)
// SSD1306 Commands - see Datasheet
#define SSD1306_CMD_START 0x00   // indicates following bytes are commands
#define SSD1306_DATA_START 0x40  // indicates following bytes are data
// Fundamental Command Table (p. 28)
#define SSD1306_SETCONTRAST 0x81        // double-byte command to set contrast (1-256)
#define SSD1306_ENTIREDISPLAY_ON 0xA5   // set entire display on
#define SSD1306_ENTIREDISPLAY_OFF 0xA4  // use RAM contents for display
#define SSD1306_SETINVERT_ON 0xA7       // invert RAM contents to display
#define SSD1306_SETINVERT_OFF 0xA6      // normal display
#define SSD1306_SETDISPLAY_OFF 0xAE     // display OFF (sleep mode)
#define SSD1306_SETDISPLAY_ON 0xAF      // display ON (normal mode)
// Scrolling Command Table (pp. 28-30)
#define SSD1306_SCROLL_SETUP_H_RIGHT 0x26   // configure right horizontal scroll
#define SSD1306_SCROLL_SETUP_H_LEFT 0x27    // configure left horizontal scroll
#define SSD1306_SCROLL_SETUP_HV_RIGHT 0x29  // configure right & vertical scroll
#define SSD1306_SCROLL_SETUP_HV_LEFT 0x2A   // configure left & vertical scroll
#define SSD1306_SCROLL_SETUP_V 0xA3         // configure vertical scroll area
#define SSD1306_SCROLL_DEACTIVATE 0x2E      // stop scrolling
#define SSD1306_SCROLL_ACTIVATE 0x2F        // start scrolling
// Addressing Setting Command Table (pp. 30-31)
#define SSD1306_PAGE_COLSTART_LOW 0x00   // set lower 4 bits of column start address by ORing 4 LSBs
#define SSD1306_PAGE_COLSTART_HIGH 0x10  // set upper 4 bits of column start address by ORing 4 LSBs
#define SSD1306_PAGE_PAGESTART 0xB0      // set page start address by ORing 4 LSBs
#define SSD1306_SETADDRESSMODE 0x20      // set addressing mode (horizontal, vertical, or page)
#define SSD1306_SETCOLRANGE 0x21         // send 2 more bytes to set start and end columns for hor/vert modes
#define SSD1306_SETPAGERANGE 0x22        // send 2 more bytes to set start and end pages
// Hardware Configuration Commands (p. 31)
#define SSD1306_SETSTARTLINE 0x40        // set RAM display start line by ORing 6 LSBs
#define SSD1306_COLSCAN_ASCENDING 0xA0   // set column address 0 to display column 0
#define SSD1306_COLSCAN_DESCENDING 0xA1  // set column address 127 to display column 127
#define SSD1306_SETMULTIPLEX 0xA8        // set size of multiplexer based on display height (31 for 32 rows)
#define SSD1306_COMSCAN_ASCENDING 0xC0   // set COM 0 to display row 0
#define SSD1306_COMSCAN_DESCENDING 0xC8  // set COM N-1 to display row 0
#define SSD1306_VERTICALOFFSET 0xD3      // set display vertical shift
#define SSD1306_SETCOMPINS 0xDA          // set COM pin hardware configuration
// Timing and Driving Scheme Settings Commands (p. 32)
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5  // set display clock divide ratio and frequency
#define SSD1306_SETPRECHARGE 0xD9        // set pre-charge period
#define SSD1306_SETVCOMLEVEL 0xDB        // set V_COMH voltage level
#define SSD1306_NOP 0xE3                 // no operation
// Charge Pump Commands (p. 62)
#define SSD1306_SETCHARGEPUMP 0x8D  // enable / disable charge pump

struct SSD1306 {
    uint32_t i2c;
    uint8_t addr;
    uint8_t width;
    uint8_t height;
    uint16_t screen_data_length;
    uint8_t *screen_data;
};

static void SSD1306_send_data(struct SSD1306 *ssd1306, int spec, uint8_t data) {
    uint8_t bf[2];
    bf[0] = spec;
    bf[1] = data;
    i2c_transfer7(ssd1306->i2c, ssd1306->addr, bf, 2, NULL, 0);
}

static void SSD1306_draw_pixel(struct SSD1306 *ssd1306, uint8_t x, uint8_t y) {
    x &= 0b01111111;  // 128
    y &= 0b00011111;  // 32
    // y = HEIGHT - 1 - (y & 0b00011111);
    ssd1306->screen_data[x + (y >> 3) * ssd1306->width] |= 1 << (y & 0x07);
}

static void SSD1306_draw_char(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, char ch) {
    uint8_t i, j;
    for(i = 0; i < 8; i++) {
        uint8_t line = font8x8_basic[(int)ch][i];
        for(j = 0; j < 8; j++, line >>= 1) {
            ssd1306->screen_data[(x + j) + ((y + i) >> 3) * ssd1306->width] |= (line & 1) << ((y + i) & 0x07);
        }
    }
}

static void SSD1306_draw_string(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, const char *str) {
    while(*str) {
        SSD1306_draw_char(ssd1306, x, y, *str++);
        x += 8;
    }
}

static void SSD1306_refresh(struct SSD1306 *ssd1306) {
    uint8_t pbuffer[WIDTH + 1];
    for(int i = 0; i < 8; i++) {
        SSD1306_send_data(ssd1306, 0x00, 0xB0 + i);
        SSD1306_send_data(ssd1306, 0x00, 0x00);
        SSD1306_send_data(ssd1306, 0x00, 0x10);

        uint8_t *buffer = ssd1306->screen_data + i * WIDTH + WIDTH - 1;
        pbuffer[0] = 0x40;
        for(int j = 0; j < WIDTH; j++) {
            pbuffer[j + 1] = *buffer;
            buffer--;
        }

        i2c_transfer7(ssd1306->i2c, ssd1306->addr, pbuffer, WIDTH + 1, NULL, 0);
    }
}

static void SSD1306_init(struct SSD1306 *ssd1306) {
    ssd1306->i2c = I2C1;
    ssd1306->addr = 0x3C;
    ssd1306->width = WIDTH;
    ssd1306->height = HEIGHT;

    ssd1306->screen_data_length = ssd1306->width * ssd1306->height >> 3;
    ssd1306->screen_data = (uint8_t *)malloc(ssd1306->screen_data_length);

    const uint8_t instructions[] = {
        SSD1306_CMD_START,            // start commands
        SSD1306_SETDISPLAY_OFF,       // turn off display
        SSD1306_SETDISPLAYCLOCKDIV,   // set clock:
        0x80,                         //   Fosc = 8, divide ratio = 0+1
        SSD1306_SETMULTIPLEX,         // display multiplexer:
        (WIDTH - 1),                  //   number of display rows
        SSD1306_VERTICALOFFSET,       // display vertical offset:
        0,                            //   no offset
        SSD1306_SETSTARTLINE | 0x00,  // RAM start line 0
        SSD1306_SETCHARGEPUMP,        // charge pump:
        0x14,                         //   charge pump ON (0x10 for OFF)
        SSD1306_SETADDRESSMODE,       // addressing mode:
        0x00,                         //   horizontal mode
        SSD1306_COLSCAN_DESCENDING,   // flip columns
        SSD1306_COMSCAN_ASCENDING,    // don't flip rows (pages)
        SSD1306_SETCOMPINS,           // set COM pins
        0x02,                         //   sequential pin mode
        SSD1306_SETCONTRAST,          // set contrast
        0x40,                         //   minimal contrast
        SSD1306_SETPRECHARGE,         // set precharge period
        0xF1,                         //   phase1 = 15, phase2 = 1
        SSD1306_SETVCOMLEVEL,         // set VCOMH deselect level
        0x40,
        SSD1306_ENTIREDISPLAY_OFF,
        SSD1306_SETINVERT_OFF,
        SSD1306_SCROLL_DEACTIVATE,
        SSD1306_SETDISPLAY_ON,
    };
    // send list of commands
    i2c_transfer7(ssd1306->i2c, ssd1306->addr, instructions, sizeof(instructions), NULL, 0);
}

static void delay(int n) {
    for(int i = 0; i < n; i++) __asm__("nop");
}

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

static uint16_t read_adc_naiive(uint8_t channel) {
    uint8_t channel_array[16];
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_start_conversion_direct(ADC1);
    while(!adc_eoc(ADC1))
        ;
    uint16_t reg16 = adc_read_regular(ADC1);
    return reg16;
}

// char* itoa(int val, char *buf, int base){
// 	int i = 30;
// 	for(; val && i ; --i, val /= base)
// 		buf[i] = "0123456789abcdef"[val % base];
// 	return &buf[i+1];
// }

static void print_number(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, int16_t num) {
    // max length => -32768
    char buf[7];
    itoa(num, buf, 10);
    buf[6] = 0;
    SSD1306_draw_string(ssd1306, x, y, buf);
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

    SSD1306_init(&ssd1306);

    memset(ssd1306.screen_data, 0x00, ssd1306.screen_data_length);
    SSD1306_refresh(&ssd1306);

    while(true) {
        uint16_t adc1 = read_adc_naiive(1);
        uint16_t adc2 = read_adc_naiive(2);

        memset(ssd1306.screen_data, 0x00, ssd1306.screen_data_length);

        SSD1306_draw_string(&ssd1306, 0, 0, "ADC1:");
        print_number(&ssd1306, 8 * 5, 0, adc1);

        SSD1306_draw_string(&ssd1306, 0, 8, "ADC2:");
        print_number(&ssd1306, 8 * 5, 8, adc2);

        int px = 90 + (adc1 / 220);
        int py = (adc2 / 220);
        SSD1306_draw_pixel(&ssd1306, px, py);
        SSD1306_draw_pixel(&ssd1306, px + 1, py + 1);
        SSD1306_draw_pixel(&ssd1306, px + 1, py);
        SSD1306_draw_pixel(&ssd1306, px, py + 1);

        SSD1306_refresh(&ssd1306);

        delay(400000);
    }

    free(ssd1306.screen_data);
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
