#include "system_state_logic.h"

RoomState evaluateSystemState(RoomState current, bool motionDetected,
                              bool inactivityTimeoutExpired)
{
    if (motionDetected)
    {
        return RoomState::ACTIVE;
    }
    if (current == RoomState::ACTIVE && inactivityTimeoutExpired)
    {
        return RoomState::INACTIVE;
    }
    return current;
}
