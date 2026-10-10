/*
 * Copyright (c) 2026
 */

#include <unity.h>

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

void do_tests_svitlobot()
{
    UNITY_BEGIN();
    unity_run_tests_by_tag("[svitlobot]", false);
    UNITY_END();
}
