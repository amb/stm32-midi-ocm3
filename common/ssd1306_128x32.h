#include <libopencm3/stm32/i2c.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    // TODO: Why does /8 cause crash here?
    uint8_t screen_data[WIDTH*HEIGHT/4];
};

void SSD1306_send_data(struct SSD1306 *ssd1306, int spec, uint8_t data);

void SSD1306_draw_pixel(struct SSD1306 *ssd1306, uint8_t x, uint8_t y);

void SSD1306_draw_char(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, char ch);

void SSD1306_draw_string(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, const char *str);

void SSD1306_print_number(struct SSD1306 *ssd1306, uint8_t x, uint8_t y, int32_t num);

void SSD1306_clear(struct SSD1306 *ssd1306, uint8_t val);

void SSD1306_refresh(struct SSD1306 *ssd1306);

void SSD1306_init(struct SSD1306 *ssd1306, uint32_t i2c_addr);
