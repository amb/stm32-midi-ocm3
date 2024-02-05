#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#include <string.h>
#include <stdlib.h>

#define WIDTH 128
#define HEIGHT 32

// Hardware description
#define SSD1306_I2C_ADDRESS 0x3C // default I2C address
#define SSD1306_PAGE_START 0
#define SSD1306_PAGE_STOP ((WIDTH / 8) - 1)
#define SSD1306_COL_START 0
#define SSD1306_COL_STOP (HEIGHT - 1)
// SSD1306 Commands - see Datasheet
#define SSD1306_CMD_START 0x00	// indicates following bytes are commands
#define SSD1306_DATA_START 0x40 // indicates following bytes are data
// Fundamental Command Table (p. 28)
#define SSD1306_SETCONTRAST 0x81	   // double-byte command to set contrast (1-256)
#define SSD1306_ENTIREDISPLAY_ON 0xA5  // set entire display on
#define SSD1306_ENTIREDISPLAY_OFF 0xA4 // use RAM contents for display
#define SSD1306_SETINVERT_ON 0xA7	   // invert RAM contents to display
#define SSD1306_SETINVERT_OFF 0xA6	   // normal display
#define SSD1306_SETDISPLAY_OFF 0xAE	   // display OFF (sleep mode)
#define SSD1306_SETDISPLAY_ON 0xAF	   // display ON (normal mode)
// Scrolling Command Table (pp. 28-30)
#define SSD1306_SCROLL_SETUP_H_RIGHT 0x26  // configure right horizontal scroll
#define SSD1306_SCROLL_SETUP_H_LEFT 0x27   // configure left horizontal scroll
#define SSD1306_SCROLL_SETUP_HV_RIGHT 0x29 // configure right & vertical scroll
#define SSD1306_SCROLL_SETUP_HV_LEFT 0x2A  // configure left & vertical scroll
#define SSD1306_SCROLL_SETUP_V 0xA3		   // configure vertical scroll area
#define SSD1306_SCROLL_DEACTIVATE 0x2E	   // stop scrolling
#define SSD1306_SCROLL_ACTIVATE 0x2F	   // start scrolling
// Addressing Setting Command Table (pp. 30-31)
#define SSD1306_PAGE_COLSTART_LOW 0x00	// set lower 4 bits of column start address by ORing 4 LSBs
#define SSD1306_PAGE_COLSTART_HIGH 0x10 // set upper 4 bits of column start address by ORing 4 LSBs
#define SSD1306_PAGE_PAGESTART 0xB0		// set page start address by ORing 4 LSBs
#define SSD1306_SETADDRESSMODE 0x20		// set addressing mode (horizontal, vertical, or page)
#define SSD1306_SETCOLRANGE 0x21		// send 2 more bytes to set start and end columns for hor/vert modes
#define SSD1306_SETPAGERANGE 0x22		// send 2 more bytes to set start and end pages
// Hardware Configuration Commands (p. 31)
#define SSD1306_SETSTARTLINE 0x40		// set RAM display start line by ORing 6 LSBs
#define SSD1306_COLSCAN_ASCENDING 0xA0	// set column address 0 to display column 0
#define SSD1306_COLSCAN_DESCENDING 0xA1 // set column address 127 to display column 127
#define SSD1306_SETMULTIPLEX 0xA8		// set size of multiplexer based on display height (31 for 32 rows)
#define SSD1306_COMSCAN_ASCENDING 0xC0	// set COM 0 to display row 0
#define SSD1306_COMSCAN_DESCENDING 0xC8 // set COM N-1 to display row 0
#define SSD1306_VERTICALOFFSET 0xD3		// set display vertical shift
#define SSD1306_SETCOMPINS 0xDA			// set COM pin hardware configuration
// Timing and Driving Scheme Settings Commands (p. 32)
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5 // set display clock divide ratio and frequency
#define SSD1306_SETPRECHARGE 0xD9		// set pre-charge period
#define SSD1306_SETVCOMLEVEL 0xDB		// set V_COMH voltage level
#define SSD1306_NOP 0xE3				// no operation
// Charge Pump Commands (p. 62)
#define SSD1306_SETCHARGEPUMP 0x8D // enable / disable charge pump

struct SSD1306
{
	uint32_t i2c;
	uint8_t addr;
	uint8_t width;
	uint8_t height;
	uint16_t screen_data_length;
	uint8_t *screen_data;
};

void SSD1306_send_data(struct SSD1306 *ssd1306, int spec, uint8_t data)
{
	uint8_t bf[2];
	bf[0] = spec;
	bf[1] = data;
	i2c_transfer7(ssd1306->i2c, ssd1306->addr, bf, 2, NULL, 0);
}

void SSD1306_draw_pixel(struct SSD1306 *ssd1306, uint8_t x, uint8_t y)
{
	x &= 0b01111111;
	y &= 0b00011111;
	ssd1306->screen_data[x + (y >> 3) * ssd1306->width] |= 1 << (y & 0x07);
}

void SSD1306_refresh(struct SSD1306 *ssd1306)
{
	uint8_t pbuffer[WIDTH + 1];
	for (int i = 0; i < 8; i++)
	{
		SSD1306_send_data(ssd1306, 0x00, 0xB0 + i);
		SSD1306_send_data(ssd1306, 0x00, 0x00);
		SSD1306_send_data(ssd1306, 0x00, 0x10);

		uint8_t *buffer = ssd1306->screen_data + i * WIDTH;
		pbuffer[0] = 0x40;
		for (int j = 0; j < WIDTH; j++)
		{
			pbuffer[j + 1] = *buffer;
			buffer++;
		}

		i2c_transfer7(ssd1306->i2c, ssd1306->addr, pbuffer, WIDTH + 1, NULL, 0);
	}
}

void SSD1306_init_B(struct SSD1306 *ssd1306)
{
	const uint8_t instructions[] = {
		SSD1306_CMD_START,			 // start commands
		SSD1306_SETDISPLAY_OFF,		 // turn off display
		SSD1306_SETDISPLAYCLOCKDIV,	 // set clock:
		0x80,						 //   Fosc = 8, divide ratio = 0+1
		SSD1306_SETMULTIPLEX,		 // display multiplexer:
		(WIDTH - 1),				 //   number of display rows
		SSD1306_VERTICALOFFSET,		 // display vertical offset:
		0,							 //   no offset
		SSD1306_SETSTARTLINE | 0x00, // RAM start line 0
		SSD1306_SETCHARGEPUMP,		 // charge pump:
		0x14,						 //   charge pump ON (0x10 for OFF)
		SSD1306_SETADDRESSMODE,		 // addressing mode:
		0x00,						 //   horizontal mode
		SSD1306_COLSCAN_DESCENDING,	 // flip columns
		SSD1306_COMSCAN_ASCENDING,	 // don't flip rows (pages)
		SSD1306_SETCOMPINS,			 // set COM pins
		0x02,						 //   sequential pin mode
		SSD1306_SETCONTRAST,		 // set contrast
		0x40,						 //   minimal contrast
		SSD1306_SETPRECHARGE,		 // set precharge period
		0xF1,						 //   phase1 = 15, phase2 = 1
		SSD1306_SETVCOMLEVEL,		 // set VCOMH deselect level
		0x40,						 //   ????? (0,2,3)
		SSD1306_ENTIREDISPLAY_OFF,	 // use RAM contents for display
		SSD1306_SETINVERT_OFF,		 // no inversion
		SSD1306_SCROLL_DEACTIVATE,	 // no scrolling
		SSD1306_SETDISPLAY_ON,		 // turn on display (normal mode)
	};
	// send list of commands
	i2c_transfer7(ssd1306->i2c, ssd1306->addr, instructions, sizeof(instructions), NULL, 0);
}

/* Wait a bit - the lazy version */
static void delay(int n)
{
	for (int i = 0; i < n; i++)
		__asm__("nop");
}

/* Initialize I2C1 interface */
static void i2c_setup(void)
{
	/* Enable GPIOB and I2C1 clocks */
	rcc_periph_clock_enable(RCC_I2C1);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_AFIO);

	/* Set alternate functions for SCL and SDA pins of I2C1 */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
				  GPIO_I2C1_SCL | GPIO_I2C1_SDA);

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

const uint8_t u8x8_font_5x7_r[764] =
	" ~\1\1\0\0\0\0\0\0\0\0\0\0^\0\0\0\0\0\0\16\0\16\0\0\0\0(|(|"
	"(\0\0\0\10T|T \0\0\0&\20\10d\0\0\0\0(T(@\0\0\0\0\0\0\16\0"
	"\0\0\0\0\0<B\0\0\0\0\0\0B<\0\0\0\0\0\0T\70T\0\0\0\0\20\20|\20"
	"\20\0\0\0\0\200` \0\0\0\0\20\20\20\20\0\0\0\0\0``\0\0\0\0\0 \20\10\4"
	"\0\0\0\0\0<B<\0\0\0\0\0D~@\0\0\0\0DbRL\0\0\0\0\42JJ\66"
	"\0\0\0\0\30\24~\20\0\0\0\0.JJ\62\0\0\0\0<JJ\60\0\0\0\0\2b\32\6"
	"\0\0\0\0\64JJ\64\0\0\0\0\14RR<\0\0\0\0\0ll\0\0\0\0\0\200l,\0"
	"\0\0\0\0\0\20(D\0\0\0\0((((\0\0\0\0\0D(\20\0\0\0\0\0\4R\14"
	"\0\0\0\0<BZ\34\0\0\0\0|\22\22|\0\0\0\0~JJ\64\0\0\0\0<BB$"
	"\0\0\0\0~BB<\0\0\0\0~JJB\0\0\0\0~\12\12\2\0\0\0\0<BRt"
	"\0\0\0\0~\10\10~\0\0\0\0\0B~B\0\0\0\0 @@>\0\0\0\0~\30$B"
	"\0\0\0\0~@@@\0\0\0\0~\14\14~\0\0\0\0~\14\60~\0\0\0\0<BB<"
	"\0\0\0\0~\22\22\14\0\0\0\0<bB\274\0\0\0\0~\22\62L\0\0\0\0$JR$"
	"\0\0\0\0\0\2~\2\0\0\0\0>@@>\0\0\0\0\36``\36\0\0\0\0~\60\60~"
	"\0\0\0\0f\30\30f\0\0\0\0\0\16p\16\0\0\0\0bRJF\0\0\0\0\0~BB"
	"\0\0\0\0\4\10\20 \0\0\0\0\0BB~\0\0\0\0\0\4\2\4\0\0\0\0@@@@"
	"\0\0\0\0\0\2\4\0\0\0\0\0\60H(x\0\0\0\0~HH\60\0\0\0\0\60HH\0"
	"\0\0\0\0\60HH~\0\0\0\0\60hX\20\0\0\0\0\20|\22\4\0\0\0\0P\250\250\230"
	"\0\0\0\0~\10\10p\0\0\0\0\0Hz@\0\0\0\0\0@\200z\0\0\0\0~\20(@"
	"\0\0\0\0\0B~@\0\0\0\0x\20\30p\0\0\0\0x\10\10p\0\0\0\0\60HH\60"
	"\0\0\0\0\370HH\60\0\0\0\0\60HH\370\0\0\0\0x\10\10\20\0\0\0\0PXh("
	"\0\0\0\0\10>H@\0\0\0\0\70@@x\0\0\0\0\0\70@\70\0\0\0\0x``x"
	"\0\0\0\0H\60\60H\0\0\0\0\30\240@\70\0\0\0\0HhXH\0\0\0\0\0\10<B"
	"\0\0\0\0\0\0~\0\0\0\0\0\0B<\10\0\0\0\0\4\2\4\2\0\0\0";

int main(void)
{
	struct SSD1306 ssd1306;

	ssd1306.i2c = I2C1;
	ssd1306.addr = 0x3C;
	ssd1306.width = WIDTH;
	ssd1306.height = HEIGHT;

	ssd1306.screen_data_length = ssd1306.width * ssd1306.height / 8;
	ssd1306.screen_data = (uint8_t *)malloc(ssd1306.screen_data_length);
	// fill with zeros
	memset(ssd1306.screen_data, 0, ssd1306.screen_data_length);

	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
	gpio_toggle(GPIOC, GPIO13);

	i2c_setup();

	// SSD1306_init(&ssd1306);
	SSD1306_init_B(&ssd1306);

	// SSD1306_fill(&ssd1306, false);
	memset(ssd1306.screen_data, 0x00, ssd1306.screen_data_length);

	int i;
	for (i = 0; i < WIDTH; i++)
	{
		SSD1306_draw_pixel(&ssd1306, i, 16);
	}
	for (i = 0; i < HEIGHT; i++)
	{
		SSD1306_draw_pixel(&ssd1306, 5, i);
	}

	SSD1306_refresh(&ssd1306);
	// SSD1306_display(&ssd1306);

	while (true)
	{
		// gpio_toggle(GPIOC, GPIO13);
		delay(8000000);
	}

	free(ssd1306.screen_data);
	return 0;
}
