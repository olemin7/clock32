#pragma once
#include <esp_event.h>

namespace lighting
{
    ESP_EVENT_DECLARE_BASE(event);
    enum
    {
        update
    };

    typedef struct
    {
        int raw;
    } update_t;

    void init();
}
