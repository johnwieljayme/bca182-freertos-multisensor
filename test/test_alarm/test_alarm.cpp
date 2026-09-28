#include <unity.h>
#include "alarm_logic.h"

void setUp(void) {}
void tearDown(void) {}

void test_below_lower_limit_alarms_low(void)
{
    TEST_ASSERT_EQUAL_INT((int)AlarmState::LOW_TEMPERATURE,
                          (int)evaluateTemperature(17.9f));
}

void test_lower_limit_is_normal(void)
{
    TEST_ASSERT_EQUAL_INT((int)AlarmState::NORMAL,
                          (int)evaluateTemperature(18.0f));
}

void test_midrange_temperature_is_normal(void)
{
    TEST_ASSERT_EQUAL_INT((int)AlarmState::NORMAL,
                          (int)evaluateTemperature(24.0f));
}

void test_upper_limit_is_normal(void)
{
    TEST_ASSERT_EQUAL_INT((int)AlarmState::NORMAL,
                          (int)evaluateTemperature(30.0f));
}

void test_above_upper_limit_alarms_high(void)
{
    TEST_ASSERT_EQUAL_INT((int)AlarmState::HIGH_TEMPERATURE,
                          (int)evaluateTemperature(30.1f));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_below_lower_limit_alarms_low);
    RUN_TEST(test_lower_limit_is_normal);
    RUN_TEST(test_midrange_temperature_is_normal);
    RUN_TEST(test_upper_limit_is_normal);
    RUN_TEST(test_above_upper_limit_alarms_high);
    return UNITY_END();
}
