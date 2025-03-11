#include "unity.h"

TEST_CASE("testname", "[module]")
{
    // Add test here
    TEST_FAIL_MESSAGE("message");
}

void tests_do()
{
    UNITY_BEGIN();
    unity_run_tests_by_tag("[module]", false);
    UNITY_END();

    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}