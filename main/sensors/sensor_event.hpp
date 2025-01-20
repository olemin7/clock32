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

    typedef struct
    {
        int raw;
        uint16_t lux;
    } lighting_t;

    constexpr auto LUX_MAX = 1000;
    constexpr auto LUX_MIN = 0;

    typedef struct
    {
        float val;
    } temperature_t, humidity_t;

}
