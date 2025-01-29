#pragma once
#include <esp_err.h>
#include <vector>
#include <array>
#include <stdint.h>
#include <string>
/*
coordinate for seg4
(0,0)          (0,31)

(7,0)          (7,31)
*/
namespace screen
{
    constexpr auto SEGMENTS = 4;
    using buffer_t = std::array<uint8_t, 8 * SEGMENTS>;
    enum justify_t
    {
        js_left,
        js_center,
        js_right,
        js_fill,
    };

    void init();
    esp_err_t print(const buffer_t &buffer);
    esp_err_t print(const std::vector<uint8_t> &image, const justify_t justify = js_left, const uint8_t offset = 0);
    esp_err_t print(const std::string str, const justify_t justify = js_left, const uint8_t offset = 0);

}; // namespace blink
