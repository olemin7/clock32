/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#include "defines.hpp"
#include "json_wrapper.hpp"
#include <esp_log.h>
#include <sstream>

namespace proto
{
    //  static const char *TAG = "proto_defines";

    bool get(const std::string &payload, ldr_t &ldr)
    {
        auto root = json_wrapper::read_root(payload);
        const auto max = root.get_field_number("max");
        const auto min = root.get_field_number("min");
        if (max && min)
        {
            ldr.max = max.value();
            ldr.min = min.value();
            return true;
        }
        return false;
    }
    std::string to_str(const ldr_t &data)
    {
        std::stringstream ss;
        ss << R"({"min":)" << data.min << R"(, "max":)" << data.max << R"(})";
        return ss.str();
    }

    bool get(const std::string &payload, display_t &data)
    {
        auto root = json_wrapper::read_root(payload);
        const auto segment_rotation = root.get_field_number("segment_rotation");
        const auto segment_upsidedown = root.get_field_bool("segment_upsidedown");
        const auto mirrored = root.get_field_bool("mirrored");
        if (segment_rotation && segment_upsidedown && mirrored)
        {
            data.segment_rotation = segment_rotation.value();
            data.segment_upsidedown = segment_upsidedown.value();
            data.mirrored = mirrored.value();

            return true;
        }
        return false;
    }

    std::string to_str(const display_t &data)
    {
        std::stringstream ss;

        ss << R"({"segment_rotation":)" << data.segment_rotation;
        ss << R"(, "segment_upsidedown":)" << data.segment_upsidedown;
        ss << R"(, "mirrored":)" << data.mirrored;
        ss << R"(})";
        return ss.str();
    }

    bool get(const std::string &payload, brightness_t &data)
    {
        auto root = json_wrapper::read_root(payload);

        const auto points = root.get_field("points");
        if (points.get_array_size().value_or(0) == 3)
        {
            for (int i = 0; i != 3; i++)
            {
                auto point = points.get_array_item(i);
                auto lighting = point.get_field_number("lighting");
                auto brightness = point.get_field_number("brightness");
                if (!lighting || !brightness)
                {
                    return false;
                }
                data.points[i].lighting = lighting.value();
                data.points[i].brightness = brightness.value();
            }
            return true;
        }
        return false;
    }

    std::string to_str(const brightness_t &data)
    {
        std::stringstream ss;

        // ss << R"({"segment_rotation":)" << data.segment_rotation;
        // ss << R"(, "segment_upsidedown":)" << data.segment_upsidedown;
        // ss << R"(, "mirrored":)" << data.mirrored;
        // ss << R"(})";
        return ss.str();
    }

    bool get(const std::string &payload, timezone_t &data)
    {
        auto root = json_wrapper::read_root(payload);
        const auto tz = root.get_field_string("tz");

        if (tz)
        {
            data.tz = tz.value();

            return true;
        }
        return false;
    }

    std::string to_str(const timezone_t &data)
    {
        std::stringstream ss;

        ss << R"({"tz":")" << data.tz;
        ss << R"("})";
        return ss.str();
    }
}
