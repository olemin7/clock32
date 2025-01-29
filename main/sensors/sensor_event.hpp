#pragma once
#include <esp_event.h>

namespace sensor_event
{
    ESP_EVENT_DECLARE_BASE(event);
    enum
    {
        ping,
        update_request,
        lighting,
        internall_temperature,
        internall_humidity
    };

    constexpr auto LUX_MAX = 1000;
    constexpr auto LUX_MIN = 0;

    typedef struct
    {
        uint16_t val;
    } lighting_t;

    typedef struct
    {
        float val;
    } temperature_t, humidity_t;

}
