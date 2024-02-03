#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void gpio_setup(void)
{
	/* Enable GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Set LED_PIN to 'output push-pull'. */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
}

static void button_setup(void)
{
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Set GPIOA0 to 'input floating'. */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO0);
}

int main(void)
{
	button_setup();
	gpio_setup();

	int i;
	bool pressed = false;
	while (1) {
		if (!gpio_get(GPIOA, GPIO0)) {
			if (pressed) {
				continue;
			} 
			gpio_toggle(GPIOC, GPIO13);
			pressed = true;
		} else {
			pressed = false;
		}

		for (i = 0; i < 100000; i++) {
			__asm__("nop");
		}
	}

	return 0;
}
