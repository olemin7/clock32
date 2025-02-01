#pragma once
#include <esp_event.h>
#include <stdint.h>

namespace lighting
{
    void init();
    void update_adc_range(uint16_t min, uint16_t max);
}
