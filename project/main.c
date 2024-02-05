#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#include <string.h>
#include <stdlib.h>

#define WIDTH 64
#define HEIGHT 32

struct SSD1306
{
	uint32_t i2c;
	uint8_t addr;
	uint8_t width;
	uint8_t height;
	uint16_t screen_data_length;
	uint8_t *screen_data;
};

enum AddressingMode
{
	Horizontal = 0b00,
	Vertical = 0b01,
	Page = 0b10,
	INVALID = 0b11
};

enum Color
{
	White = 0,
	Black = 1
};

enum WrapType
{
	NoWrap,
	WrapDisplay,
	WrapCoord
};

enum PacketType
{
	Command = 0x00,
	Data = 0x40
};

void SSD1306_send_data(struct SSD1306 *ssd1306, int spec, uint8_t data)
{
	uint8_t bf[2];
	bf[0] = spec;
	bf[1] = data;
	i2c_transfer7(ssd1306->i2c, ssd1306->addr, bf, 2, NULL, 0);
}

void SSD1306_draw_pixel(struct SSD1306 *ssd1306, int x, int y, bool color)
{
	if (x < 0 || x >= ssd1306->width || y < 0 || y >= ssd1306->height)
	{
		return;
	}

	ssd1306->screen_data[x + (y >> 3) * ssd1306->width] = color ? 255 : 0;
}

void SSD1306_fill(struct SSD1306 *ssd1306, bool color)
{
	uint8_t fill = color ? 0xFF : 0x00;
	for (int i = 0; i < ssd1306->screen_data_length; i++)
	{
		ssd1306->screen_data[i] = fill;
	}
}

void SSD1306_refresh(struct SSD1306 *ssd1306)
{
	uint8_t pbuffer[WIDTH + 1];
	for (int i=0; i < 8; i++) {
		SSD1306_send_data(ssd1306, Command, 0xB0 + i);
		SSD1306_send_data(ssd1306, Command, 0x00);
		SSD1306_send_data(ssd1306, Command, 0x10);

		uint8_t *buffer = ssd1306->screen_data + i * WIDTH;
		pbuffer[0] = Data;
		for (int j = 0; j < WIDTH; j++)
		{
			pbuffer[j + 1] = *buffer;
			buffer++;
		}

		i2c_transfer7(ssd1306->i2c, ssd1306->addr, pbuffer, WIDTH + 1, NULL, 0);
	}
}

void SSD1306_display(struct SSD1306 *ssd1306)
{
	// Page address
	SSD1306_send_data(ssd1306, Command, 0x22);
	SSD1306_send_data(ssd1306, Command, 0x00);
	SSD1306_send_data(ssd1306, Command, 0xFF);
	// Column address
	SSD1306_send_data(ssd1306, Command, 0x21);
	SSD1306_send_data(ssd1306, Command, 0x00);

	uint16_t count = ssd1306->screen_data_length;
	uint8_t *buffer = ssd1306->screen_data;

	SSD1306_send_data(ssd1306, Command, 0x40);
	while (count--)
	{
		SSD1306_send_data(ssd1306, Data, *buffer++);
	}
}

void SSD1306_switch(struct SSD1306 *ssd1306, bool state)
{
	if (state)
	{
		SSD1306_send_data(ssd1306, Command, 0xAF);
	}
	else
	{
		SSD1306_send_data(ssd1306, Command, 0xAE);
	}
}

void SSD1306_set_addressing_mode(struct SSD1306 *ssd1306, int mode)
{
	SSD1306_send_data(ssd1306, Command, 0x20);
	SSD1306_send_data(ssd1306, Command, mode);
}

void SSD1306_init(struct SSD1306 *ssd1306)
{
	SSD1306_switch(ssd1306, false);

	SSD1306_set_addressing_mode(ssd1306, 0x00);
	
	// Set page start address for page addressing mode
	SSD1306_send_data(ssd1306, Command, 0xB0);

	// Set segment re-map
	SSD1306_send_data(ssd1306, Command, 0xA0 | 0x1);

	// Set multiplex ratio
	SSD1306_send_data(ssd1306, Command, 0xA8);
	SSD1306_send_data(ssd1306, Command, HEIGHT-1);
	
	// COM output scan direction (works)
	SSD1306_send_data(ssd1306, Command, 0xC8);
	
	// Set low column address
	SSD1306_send_data(ssd1306, Command, 0x00);

	// Set high column address
	SSD1306_send_data(ssd1306, Command, 0x10);

	// Set display start line
	SSD1306_send_data(ssd1306, Command, 0x40 | 0x0);

	// Set contrast control register
	// (works)
	SSD1306_send_data(ssd1306, Command, 0x81);
	SSD1306_send_data(ssd1306, Command, 0x8F);

	// Set display offset
	SSD1306_send_data(ssd1306, Command, 0xD3);
	SSD1306_send_data(ssd1306, Command, 0x00);

	// Set display clock divide ratio/oscillator frequency
	SSD1306_send_data(ssd1306, Command, 0xD5);
	SSD1306_send_data(ssd1306, Command, 0x80);
	// SSD1306_send_data(ssd1306, Command, 0xF0);

	// Set pre-charge period
	SSD1306_send_data(ssd1306, Command, 0xD9);
	SSD1306_send_data(ssd1306, Command, 0x22);

	// Set com pins hardware configuration
	SSD1306_send_data(ssd1306, Command, 0xDA);
	SSD1306_send_data(ssd1306, Command, (0b00110010 & 0x12));
	// SSD1306_send_data(ssd1306, Command, (0b00110010 & 0x12));

	// Set VCOMH Deselect Level
	SSD1306_send_data(ssd1306, Command, 0xDB);
	SSD1306_send_data(ssd1306, Command, 0x40);

	// Internal reference during display on
	SSD1306_send_data(ssd1306, Command, 0xAD);
	SSD1306_send_data(ssd1306, Command, 0x30);

	// Charge pump (charge on)
	SSD1306_send_data(ssd1306, Command, 0x8D);
	SSD1306_send_data(ssd1306, Command, 0x14);

	// Set all resume / entire on
	SSD1306_send_data(ssd1306, Command, 0xA4);

	// Normal display (non-inverted)
	SSD1306_send_data(ssd1306, Command, 0xA6);

	// Turn off scroll
	SSD1306_send_data(ssd1306, Command, 0x2E);

	// Turn on the display
	SSD1306_switch(ssd1306, true);
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

// const uint8_t u8x8_font_5x7_r[764] U8X8_FONT_SECTION("u8x8_font_5x7_r") =
// 	" ~\1\1\0\0\0\0\0\0\0\0\0\0^\0\0\0\0\0\0\16\0\16\0\0\0\0(|(|"
// 	"(\0\0\0\10T|T \0\0\0&\20\10d\0\0\0\0(T(@\0\0\0\0\0\0\16\0"
// 	"\0\0\0\0\0<B\0\0\0\0\0\0B<\0\0\0\0\0\0T\70T\0\0\0\0\20\20|\20"
// 	"\20\0\0\0\0\200` \0\0\0\0\20\20\20\20\0\0\0\0\0``\0\0\0\0\0 \20\10\4"
// 	"\0\0\0\0\0<B<\0\0\0\0\0D~@\0\0\0\0DbRL\0\0\0\0\42JJ\66"
// 	"\0\0\0\0\30\24~\20\0\0\0\0.JJ\62\0\0\0\0<JJ\60\0\0\0\0\2b\32\6"
// 	"\0\0\0\0\64JJ\64\0\0\0\0\14RR<\0\0\0\0\0ll\0\0\0\0\0\200l,\0"
// 	"\0\0\0\0\0\20(D\0\0\0\0((((\0\0\0\0\0D(\20\0\0\0\0\0\4R\14"
// 	"\0\0\0\0<BZ\34\0\0\0\0|\22\22|\0\0\0\0~JJ\64\0\0\0\0<BB$"
// 	"\0\0\0\0~BB<\0\0\0\0~JJB\0\0\0\0~\12\12\2\0\0\0\0<BRt"
// 	"\0\0\0\0~\10\10~\0\0\0\0\0B~B\0\0\0\0 @@>\0\0\0\0~\30$B"
// 	"\0\0\0\0~@@@\0\0\0\0~\14\14~\0\0\0\0~\14\60~\0\0\0\0<BB<"
// 	"\0\0\0\0~\22\22\14\0\0\0\0<bB\274\0\0\0\0~\22\62L\0\0\0\0$JR$"
// 	"\0\0\0\0\0\2~\2\0\0\0\0>@@>\0\0\0\0\36``\36\0\0\0\0~\60\60~"
// 	"\0\0\0\0f\30\30f\0\0\0\0\0\16p\16\0\0\0\0bRJF\0\0\0\0\0~BB"
// 	"\0\0\0\0\4\10\20 \0\0\0\0\0BB~\0\0\0\0\0\4\2\4\0\0\0\0@@@@"
// 	"\0\0\0\0\0\2\4\0\0\0\0\0\60H(x\0\0\0\0~HH\60\0\0\0\0\60HH\0"
// 	"\0\0\0\0\60HH~\0\0\0\0\60hX\20\0\0\0\0\20|\22\4\0\0\0\0P\250\250\230"
// 	"\0\0\0\0~\10\10p\0\0\0\0\0Hz@\0\0\0\0\0@\200z\0\0\0\0~\20(@"
// 	"\0\0\0\0\0B~@\0\0\0\0x\20\30p\0\0\0\0x\10\10p\0\0\0\0\60HH\60"
// 	"\0\0\0\0\370HH\60\0\0\0\0\60HH\370\0\0\0\0x\10\10\20\0\0\0\0PXh("
// 	"\0\0\0\0\10>H@\0\0\0\0\70@@x\0\0\0\0\0\70@\70\0\0\0\0x``x"
// 	"\0\0\0\0H\60\60H\0\0\0\0\30\240@\70\0\0\0\0HhXH\0\0\0\0\0\10<B"
// 	"\0\0\0\0\0\0~\0\0\0\0\0\0B<\10\0\0\0\0\4\2\4\2\0\0\0";

int main(void)
{
	struct SSD1306 ssd1306;

	ssd1306.i2c = I2C1;
	ssd1306.addr = 0x3C;
	ssd1306.width = WIDTH;
	ssd1306.height = HEIGHT;

	ssd1306.screen_data_length = ssd1306.width * ssd1306.height >> 3;
	ssd1306.screen_data = (uint8_t *)malloc(ssd1306.screen_data_length);
	// fill with zeros
	memset(ssd1306.screen_data, 0, ssd1306.screen_data_length);

	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
	gpio_toggle(GPIOC, GPIO13);

	i2c_setup();

	SSD1306_init(&ssd1306);

	// SSD1306_fill(&ssd1306, false);
	memset(ssd1306.screen_data, 0x0, ssd1306.screen_data_length);

	// SSD1306_draw_pixel(&ssd1306, 10, 10, true);
	// SSD1306_draw_pixel(&ssd1306, 20, 20, true);

	int i;
	for (i=0;i<WIDTH;i++) {
		SSD1306_draw_pixel(&ssd1306, i, 16, true);
	}
	for (i=0;i<HEIGHT;i++) {
		SSD1306_draw_pixel(&ssd1306, 32, i, true);
	}

	// delay(8000000);

	SSD1306_refresh(&ssd1306);
	// SSD1306_display(&ssd1306);

	while (true)
	{
		gpio_toggle(GPIOC, GPIO13);
		delay(8000000);
	}

	free(ssd1306.screen_data);
	return 0;
}
