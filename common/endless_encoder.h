#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct EndlessEncoder {
    uint16_t smooth1;
    uint16_t smooth2;
    int16_t base_count;
    uint8_t previous_sector;
} EndlessEncoder;

int32_t encoder_get_total(EndlessEncoder *enc);

bool encoder_update_smooth(EndlessEncoder *enc, uint16_t adc1, uint16_t adc2);
