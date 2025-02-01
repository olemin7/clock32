/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <string>
#include <inttypes.h>
#include "handler.hpp"
namespace proto
{
    struct ldr_t
    {
        uint16_t max;
        uint16_t min;
    };

    bool get(cJSON_opt_t payload, ldr_t &data);

    struct display_t
    {
        uint8_t segment_rotation;
        bool segment_upsidedown;
        bool mirrored;
    };
    bool get(cJSON_opt_t payload, display_t &data);

} // namespace utils
