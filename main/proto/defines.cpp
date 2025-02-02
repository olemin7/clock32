/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#include "defines.hpp"
#include <esp_log.h>

namespace proto
{
    //  static const char *TAG = "proto_defines";

    bool get(cJSON_opt_t payload, ldr_t &ldr)
    {

        const auto max = get_field_number(payload, "max");
        const auto min = get_field_number(payload, "min");
        if (max && min)
        {
            ldr.max = max.value();
            ldr.min = min.value();
            return true;
        }
        return false;
    }

    bool get(cJSON_opt_t payload, display_t &data)
    {
        const auto segment_rotation = get_field_number(payload, "segment_rotation");
        const auto segment_upsidedown = get_field_bool(payload, "segment_upsidedown");
        const auto mirrored = get_field_bool(payload, "mirrored");
        if (segment_rotation && segment_upsidedown && mirrored)
        {
            data.segment_rotation = segment_rotation.value();
            data.segment_upsidedown = segment_upsidedown.value();
            data.mirrored = mirrored.value();

            return true;
        }
        return false;
    }

    bool get(cJSON_opt_t payload, brightness_t &data)
    {
        const auto min = get_field_number(payload, "min");
        const auto max = get_field_number(payload, "max");

        if (min && max)
        {
            data.min = min.value();
            data.max = max.value();

            return true;
        }
        return false;
    }

    bool get(cJSON_opt_t payload, timezone_t &data)
    {
        const auto tz = get_field_string(payload, "tz");

        if (tz)
        {
            data.tz = tz.value();

            return true;
        }
        return false;
    }
}
