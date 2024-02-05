#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <u8x8.h>

/* Wait a bit - the lazy version */
static void delay(int n) {
	for(int i = 0; i < n; i++)
		__asm__("nop");
}

/* Initialize I2C1 interface */
static void i2c_setup(void) {
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

	// while (I2C_CR1(I2C1) & I2C_CR1_PE != 0);
}

static uint8_t u8x8_gpio_and_delay_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch(msg) {
	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		i2c_setup();  /* Init I2C communication */
		break;

	default:
		u8x8_SetGPIOResult(u8x8, 1);
		break;
	}

	return 1;
}

/* I2C hardware transfer based on u8x8_byte.c implementation */
static uint8_t u8x8_byte_hw_i2c_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	static uint8_t buffer[32];   /* u8g2/u8x8 will never send more than 32 bytes */
	static uint8_t buf_idx;
	uint8_t *data;

	switch(msg) {
	case U8X8_MSG_BYTE_SEND:
		data = (uint8_t *)arg_ptr;
		while(arg_int > 0) {
			buffer[buf_idx++] = *data;
			data++;
			arg_int--;
		}
		break;
	case U8X8_MSG_BYTE_INIT:
		break;
	case U8X8_MSG_BYTE_SET_DC:
		break;
	case U8X8_MSG_BYTE_START_TRANSFER:
		buf_idx = 0;
		break;
	case U8X8_MSG_BYTE_END_TRANSFER:
		i2c_transfer7(I2C1, 0x3C, buffer, buf_idx, NULL, 0);
		break;
	default:
		return 0;
	}
	return 1;
}

const uint8_t u8x8_font_5x7_r[764] U8X8_FONT_SECTION("u8x8_font_5x7_r") = 
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

int main(void) {
	u8x8_t u8x8_i, *u8x8 = &u8x8_i;

	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
	gpio_toggle(GPIOC, GPIO13);

	delay(100000);

	// u8x8_Setup(u8x8, u8x8_d_ssd1306_64x32_1f, u8x8_cad_ssd13xx_fast_i2c, u8x8_byte_hw_i2c_cm3, u8x8_gpio_and_delay_cm3);
	u8x8_Setup(u8x8, u8x8_d_ssd1306_64x32_noname, u8x8_cad_ssd13xx_fast_i2c, u8x8_byte_hw_i2c_cm3, u8x8_gpio_and_delay_cm3);

	u8x8_InitDisplay(u8x8);
	u8x8_SetPowerSave(u8x8, 0);
	u8x8_SetFont(u8x8, u8x8_font_5x7_r);

	delay(8000000);

	u8x8_ClearDisplay(u8x8);
	u8x8_DrawString(u8x8, 1, 1, "Hello!!!");
	u8x8_Draw2x2Glyph(u8x8, 0, 0, 'H');
	u8x8_SetInverseFont(u8x8, 1);
	u8x8_DrawString(u8x8, 0, 5, "Roel says hi!  ");
	u8x8_SetInverseFont(u8x8, 0);

	while(true) {
		gpio_toggle(GPIOC, GPIO13);
		delay(8000000);
	}

	return 0;
}

