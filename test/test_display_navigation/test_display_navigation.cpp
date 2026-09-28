#include <unity.h>
#include "display_logic.h"

void setUp(void) {}
void tearDown(void) {}

void test_clockwise_advances_to_humidity(void)
{
    TEST_ASSERT_EQUAL_INT((int)DisplayMode::HUMIDITY,
                          (int)nextDisplayMode(DisplayMode::TEMPERATURE));
}

void test_clockwise_wraps_to_temperature(void)
{
    TEST_ASSERT_EQUAL_INT((int)DisplayMode::TEMPERATURE,
                          (int)nextDisplayMode(DisplayMode::MOTION));
}

void test_counterclockwise_moves_to_previous_mode(void)
{
    TEST_ASSERT_EQUAL_INT((int)DisplayMode::LIGHT,
                          (int)previousDisplayMode(DisplayMode::MOTION));
}

void test_counterclockwise_wraps_to_motion(void)
{
    TEST_ASSERT_EQUAL_INT((int)DisplayMode::MOTION,
                          (int)previousDisplayMode(DisplayMode::TEMPERATURE));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clockwise_advances_to_humidity);
    RUN_TEST(test_clockwise_wraps_to_temperature);
    RUN_TEST(test_counterclockwise_moves_to_previous_mode);
    RUN_TEST(test_counterclockwise_wraps_to_motion);
    return UNITY_END();
}
