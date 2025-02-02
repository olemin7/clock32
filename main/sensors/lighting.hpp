#pragma once
#include <esp_event.h>
#include <stdint.h>

namespace lighting
{
    void init();
    void update_adc_range(int min, int max);
}
