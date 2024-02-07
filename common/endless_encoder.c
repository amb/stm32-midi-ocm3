#include "endless_encoder.h"

void encoder_process(EndlessEncoder *enc) {
    // calc sectors (0 to 3 in clockwise order)
    uint8_t sectors = (enc->smooth1 > 2048) | ((enc->smooth2 > 2048) << 1);
    sectors ^= (sectors > 1);

    // keep track of 360 rotations
    if(enc->previous_sector == 0 && sectors == 3) { enc->base_count--; }
    if(enc->previous_sector == 3 && sectors == 0) { enc->base_count++; }

    // calculate the total value for knob
    // 8192 is one full rotation
    // interpolate between the two smooth normalized inputs and add to total
    int32_t vpadc1 = enc->smooth1;
    int32_t vpadc2 = enc->smooth2 - 2048;
    if(sectors == 2 || sectors == 3) { vpadc1 = 8192 - vpadc1; }
    if(sectors == 3 || sectors == 0) {
        // & 0x1FFF is same as % 8192
        vpadc2 = (8192 - vpadc2) & 0x1FFF;
    } else {
        vpadc2 += 4096;
    }

    int32_t ratio = abs(enc->smooth1 - 2048);
    enc->previous_total_value = enc->total_value;
    enc->total_value = (enc->base_count * 8192) + ((vpadc1 * (2048 - ratio) + vpadc2 * ratio) >> 11);

    enc->previous_sector = sectors;
}

int32_t encoder_get_relative(EndlessEncoder *enc) {
    return enc->total_value - enc->previous_total_value;
}

bool encoder_update(EndlessEncoder *enc, uint16_t adc1, uint16_t adc2) {
    bool updated = false;

    // lag filtering
    // TODO: 10 range might not be enough, try 16 (test it)
    if(abs(enc->smooth1 - adc1) > 10) {
        enc->smooth1 = adc1 - (adc1 > enc->smooth1) * 10;
        updated = true;
    }

    if(abs(enc->smooth2 - adc2) > 10) {
        enc->smooth2 = adc2 - (adc2 > enc->smooth2) * 10;
        updated = true;
    }

    if (updated) {
        encoder_process(enc);
    }

    return updated;
}
