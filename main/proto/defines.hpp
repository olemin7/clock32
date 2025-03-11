/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once

#include <string>
#include <inttypes.h>

namespace proto
{
    struct ldr_t
    {
        int max;
        int min;
    };

    bool get(const std::string &payload, ldr_t &data);
    std::string to_str(const ldr_t &data);

    struct display_t
    {
        uint8_t segment_rotation;
        bool segment_upsidedown;
        bool mirrored;
    };
    bool get(const std::string &payload, display_t &data);
    std::string to_str(const display_t &data);

    struct brightness_t
    {
        struct point_t
        {
            uint16_t lighting;
            uint8_t brightness;
        };
        point_t points[3];
    };

    bool get(const std::string &payload, brightness_t &data);

    struct timezone_t
    {
        std::string tz;
    };
    bool get(const std::string &payload, timezone_t &data);
    std::string to_str(const timezone_t &data);

} // namespace utils
