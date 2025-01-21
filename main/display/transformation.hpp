#pragma once
#include <stdint.h>
#include <vector>
#include <array>
#include "screen.hpp"

namespace transformation
{

    uint8_t reverse_bits(uint8_t num);
    screen::buffer_t buffer_by_segment_rotate(const screen::buffer_t &in, uint8_t rotation);
    screen::buffer_t get_test_buffer();

    enum justify_t
    {
        js_left,
        js_center,
        js_right
    };
    screen::buffer_t
    image2buff(const std::vector<uint8_t> &image, const uint8_t offset = 0, const justify_t justify = js_left);
}