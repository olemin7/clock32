#pragma once
#include <stdint.h>
#include <functional>
#include "esp_timer_cxx.hpp"

namespace lighting
{
    class lighting
    {
    public:
        using cb_t = std::function<void(uint8_t const)>;
        lighting();
        uint8_t get() const
        {
            return adc_val_;
        }

    private:
        idf::esp_timer::ESPTimer timer_;
        uint16_t adc_val_;
        cb_t bc_;
    };
}
