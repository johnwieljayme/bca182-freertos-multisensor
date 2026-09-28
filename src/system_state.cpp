#include "system_state.h"

#include "FreeRTOS.h"
#include "task.h"
#include "rtos_objects.h"
#include "serial_log.h"

static constexpr uint32_t kStatePeriodMs = 100;

void StateTask(void *pvParameters)
{
    (void)pvParameters;
    RoomState state = RoomState::ACTIVE;
    TickType_t lastMotion = xTaskGetTickCount();
    log_line("StateTask started");

    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kStatePeriodMs));
        bool motion = (xEventGroupGetBits(xSystemEvents) & EVENT_MOTION) != 0;
        if (motion)
        {
            lastMotion = xTaskGetTickCount();
        }

        bool timeout = (xTaskGetTickCount() - lastMotion) >=
                       pdMS_TO_TICKS(kInactivityTimeoutMs);
        RoomState next = evaluateSystemState(state, motion, timeout);
        if (next != state)
        {
            state = next;
            if (state == RoomState::ACTIVE)
            {
                xEventGroupSetBits(xSystemEvents, EVENT_ACTIVE);
                log_line("STATE: ACTIVE (motion)");
            }
            else
            {
                xEventGroupClearBits(xSystemEvents, EVENT_ACTIVE);
                log_line("STATE: INACTIVE (no motion for 15 s)");
            }
        }
    }
}
