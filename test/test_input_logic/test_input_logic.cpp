#include <unity.h>
#include "input_logic.h"

void setUp(void) {}
void tearDown(void) {}

void test_clockwise_falling_edge(void)
{
    TEST_ASSERT_EQUAL_INT(1, encoder_step(true, false, true));
}

void test_counterclockwise_falling_edge(void)
{
    TEST_ASSERT_EQUAL_INT(-1, encoder_step(true, false, false));
}

void test_rising_edge_does_not_step(void)
{
    TEST_ASSERT_EQUAL_INT(0, encoder_step(false, true, false));
}

void test_active_low_button(void)
{
    TEST_ASSERT_TRUE(encoder_button_pressed(false));
    TEST_ASSERT_FALSE(encoder_button_pressed(true));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clockwise_falling_edge);
    RUN_TEST(test_counterclockwise_falling_edge);
    RUN_TEST(test_rising_edge_does_not_step);
    RUN_TEST(test_active_low_button);
    return UNITY_END();
}
