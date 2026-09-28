#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "system_state_logic.h"

constexpr uint32_t kInactivityTimeoutMs = 15000;

RoomState evaluateSystemState(RoomState current, bool motionDetected,
                              bool timeoutElapsed);
void StateTask(void *pvParameters);

#endif
