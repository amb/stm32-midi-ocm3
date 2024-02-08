#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>

#include "udelay.h"
#include "i2s_spi.h"

uint16_t phase[4] = {0};
uint16_t sample = 0;

void update_sample(void);

// I2S SPI uses TIM3 and SPI1
void tim3_isr() {
    if(timer_get_flag(TIM3, TIM_SR_CC1IF)) {
        timer_clear_flag(TIM3, TIM_SR_CC1IF);
        update_sample();
    }
}

void update_sample() {
    phase[0] += 220;
    phase[1] += 549;
    phase[2] += 661;

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
    sample = phase[0];

    i2s_send(sample);
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    delay_setup();
    i2s_spi_setup();

    while(1) {
        // Main loop
    }

    return 0;
}
