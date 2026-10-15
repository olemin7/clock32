/*
 * Copyright (c) 2026
 */

#include <unity.h>

#include "sdkconfig.h"

#include "../svitlobot.hpp"

TEST_CASE("make_url", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_STRING("https://host/channelPing?channel_key=ABC",
                             svitlobot::make_url("https://host/channelPing", "ABC").c_str());
}

TEST_CASE("mask_key_long", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_STRING("AB******IJ", svitlobot::mask_key("ABCDEFGHIJ").c_str());
    TEST_ASSERT_EQUAL_STRING("AB*DE", svitlobot::mask_key("ABCDE").c_str());
}

TEST_CASE("mask_key_short", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_STRING("****", svitlobot::mask_key("ABCD").c_str());
    TEST_ASSERT_EQUAL_STRING("*", svitlobot::mask_key("A").c_str());
}

TEST_CASE("mask_key_empty", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_STRING("", svitlobot::mask_key("").c_str());
}

TEST_CASE("retry_delay_first", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_UINT32(5000, svitlobot::retry_delay_ms(0));
}

TEST_CASE("retry_delay_backoff", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_UINT32(10000, svitlobot::retry_delay_ms(5000));
    TEST_ASSERT_EQUAL_UINT32(20000, svitlobot::retry_delay_ms(10000));
}

TEST_CASE("retry_delay_capped", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_UINT32(20000, svitlobot::retry_delay_ms(20000));
    TEST_ASSERT_EQUAL_UINT32(20000, svitlobot::retry_delay_ms(15000));
    // No overflow on an absurd input.
    TEST_ASSERT_EQUAL_UINT32(20000, svitlobot::retry_delay_ms(0xFFFFFFFFu));
}

// A failed ping plus the whole retry chain must stay under the interval, so a
// burst of failures can never produce a multi-minute hole in the channel.
TEST_CASE("retry_delay_fits_interval", "[svitlobot]")
{
    uint32_t delay = svitlobot::retry_delay_ms(0);
    TEST_ASSERT_TRUE(delay < CONFIG_SVITLOBOT_INTERVAL_S * 1000);
    TEST_ASSERT_TRUE(CONFIG_SVITLOBOT_TIMEOUT_S * 1000 + delay < CONFIG_SVITLOBOT_INTERVAL_S * 1000);
}

// The API records one ping per minute per channel, so an interval at or below
// 60 s would get a large share of pings bounced with HTTP 429 for no reason.
TEST_CASE("interval_above_api_rate_limit", "[svitlobot]")
{
    TEST_ASSERT_TRUE(CONFIG_SVITLOBOT_INTERVAL_S > 60);
}

TEST_CASE("startup_jitter_in_window", "[svitlobot]")
{
    constexpr uint32_t window_ms = CONFIG_SVITLOBOT_STARTUP_JITTER_S * 1000;
    for (uint32_t value : {0u, 1u, 12345u, 999983u, 0xFFFFFFFFu})
    {
        const uint32_t jitter = svitlobot::startup_jitter_ms(value);
        if (window_ms == 0)
        {
            TEST_ASSERT_EQUAL_UINT32(0, jitter);
        }
        else
        {
            TEST_ASSERT_TRUE(jitter < window_ms);
        }
    }
}

TEST_CASE("startup_jitter_zero_input", "[svitlobot]")
{
    TEST_ASSERT_EQUAL_UINT32(0, svitlobot::startup_jitter_ms(0));
}

// Jitter must not be able to delay the first ping past the point where the
// channel would already be considered offline. Also enforced by a static_assert
// in svitlobot.cpp, kept here so a bad menuconfig combination is visible.
TEST_CASE("startup_jitter_below_interval", "[svitlobot]")
{
    TEST_ASSERT_TRUE(CONFIG_SVITLOBOT_STARTUP_JITTER_S <= CONFIG_SVITLOBOT_INTERVAL_S);
}

void do_tests_svitlobot()
{
    UNITY_BEGIN();
    unity_run_tests_by_tag("[svitlobot]", false);
    UNITY_END();
}
