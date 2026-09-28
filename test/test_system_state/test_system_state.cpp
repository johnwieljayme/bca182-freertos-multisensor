#include <unity.h>
#include "system_state_logic.h"

void setUp(void) {}
void tearDown(void) {}

void test_active_without_timeout_stays_active(void)
{
    TEST_ASSERT_EQUAL_INT((int)RoomState::ACTIVE,
                          (int)evaluateSystemState(RoomState::ACTIVE, false, false));
}

void test_active_timeout_becomes_inactive(void)
{
    TEST_ASSERT_EQUAL_INT((int)RoomState::INACTIVE,
                          (int)evaluateSystemState(RoomState::ACTIVE, false, true));
}

void test_inactive_without_motion_stays_inactive(void)
{
    TEST_ASSERT_EQUAL_INT((int)RoomState::INACTIVE,
                          (int)evaluateSystemState(RoomState::INACTIVE, false, false));
}

void test_motion_reactivates_system(void)
{
    TEST_ASSERT_EQUAL_INT((int)RoomState::ACTIVE,
                          (int)evaluateSystemState(RoomState::INACTIVE, true, false));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_active_without_timeout_stays_active);
    RUN_TEST(test_active_timeout_becomes_inactive);
    RUN_TEST(test_inactive_without_motion_stays_inactive);
    RUN_TEST(test_motion_reactivates_system);
    return UNITY_END();
}
