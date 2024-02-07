#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct EndlessEncoder {
    int32_t total_value;
    int32_t previous_total_value;
    uint16_t smooth1;
    uint16_t smooth2;
    int16_t base_count;
    uint8_t previous_sector;
    uint8_t divider;
} EndlessEncoder;

void encoder_process(EndlessEncoder *enc);

int32_t encoder_get_relative(EndlessEncoder *enc);

bool encoder_update(EndlessEncoder *enc, uint16_t adc1, uint16_t adc2);
