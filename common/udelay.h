#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

// Uses TIM2

void delay_setup(void);

void delay_us(uint32_t us);
