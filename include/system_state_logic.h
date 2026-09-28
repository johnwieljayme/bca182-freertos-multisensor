#ifndef SYSTEM_STATE_LOGIC_H
#define SYSTEM_STATE_LOGIC_H

enum class RoomState
{
    ACTIVE,
    INACTIVE
};

RoomState evaluateSystemState(RoomState current, bool motionDetected,
                              bool inactivityTimeoutExpired);

#endif
