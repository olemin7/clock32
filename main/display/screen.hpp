#pragma once
#include <esp_err.h>
#include <vector>
#include <array>
#include <stdint.h>
namespace screen
{
    constexpr auto SEGMENTS = 4;
    using buffer_t = std::array<uint8_t, 8 * SEGMENTS>;

    void init();
    esp_err_t image(const std::vector<uint8_t> &image);

}; // namespace blink