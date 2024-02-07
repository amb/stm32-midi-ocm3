#include "ssd1306_128x32.h"

#include "font8x8_basic.h"
#include "tools.h"

void SSD1306_send_data(struct SSD1306 *ssd1306, int spec, uint8_t data) {
    uint8_t bf[2];
    bf[0] = spec;
    bf[1] = data;
    i2c_transfer7(ssd1306->i2c, ssd1306->addr, bf, 2, NULL, 0);
}

void SSD1306_draw_pixel(struct SSD1306 *ssd1306, uint8_t x, uint8_t y) {
    x &= 0b01111111;  // 128
    y &= 0b00011111;  // 32
    // y = HEIGHT - 1 - (y & 0b00011111);
    ssd1306->screen_data[(x + (y >> 3) * ssd1306->width) & 0xFFF] |= 1 << (y & 0x07);
}

void SSD1306_draw_char(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, char ch) {
    uint8_t i, j;
    uint8_t selchar = ((int)ch) - 32;
    for(i = 0; i < 8; i++) {
        uint8_t line = font8x8_basic[selchar][i];
        uint8_t y_seg = (y + i) & 0x07;
        for(j = 0; j < 8; j++, line >>= 1) {
            ssd1306->screen_data[((x + j) + ((y + i) >> 3) * ssd1306->width) & 0xFFF] |= (line & 1) << y_seg;
        }
    }
}

void SSD1306_draw_string(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, const char *str) {
    while(*str) {
        SSD1306_draw_char(ssd1306, x, y, *str++);
        x += 8;
    }
}

void SSD1306_print_number(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, int32_t num) {
    // max length => -32768
    char buf[17];
    itoa7(num, buf);
    buf[16] = 0;
    SSD1306_draw_string(ssd1306, x, y, buf);
}

void SSD1306_clear(struct SSD1306 *ssd1306, uint8_t val) {
    memset(ssd1306->screen_data, val, ssd1306->screen_data_length);
}

void SSD1306_refresh(struct SSD1306 *ssd1306) {
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

void SSD1306_init(struct SSD1306 *ssd1306, uint32_t i2c_addr) {
    ssd1306->i2c = i2c_addr;
    ssd1306->addr = 0x3C;
    ssd1306->width = WIDTH;
    ssd1306->height = HEIGHT;

    ssd1306->screen_data_length = ssd1306->width * ssd1306->height >> 3;
    // TODO: is using malloc here reasonable?
    //       it eats 600 bytes from the firmware
    // ssd1306->screen_data = (uint8_t *)malloc(ssd1306->screen_data_length);

    const uint8_t instructions[] = {
        SSD1306_CMD_START,
        SSD1306_SETDISPLAY_OFF,
        SSD1306_SETDISPLAYCLOCKDIV,
        // Fosc = 8, divide ratio = 0 + 1
        0x80,
        SSD1306_SETMULTIPLEX,
        (WIDTH - 1),
        SSD1306_VERTICALOFFSET,
        0,
        SSD1306_SETSTARTLINE | 0x00,
        SSD1306_SETCHARGEPUMP,
        0x14,
        SSD1306_SETADDRESSMODE,
        0x00,
        SSD1306_COLSCAN_DESCENDING,
        SSD1306_COMSCAN_ASCENDING,
        SSD1306_SETCOMPINS,
        0x02,
        SSD1306_SETCONTRAST,
        0x40,
        SSD1306_SETPRECHARGE,
        0xF1,
        SSD1306_SETVCOMLEVEL,
        0x40,
        SSD1306_ENTIREDISPLAY_OFF,
        SSD1306_SETINVERT_OFF,
        SSD1306_SCROLL_DEACTIVATE,
        SSD1306_SETDISPLAY_ON,
    };

    // send list of commands
    i2c_transfer7(ssd1306->i2c, ssd1306->addr, instructions, sizeof(instructions), NULL, 0);
}
