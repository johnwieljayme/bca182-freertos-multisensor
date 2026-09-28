#include "rtos_objects.h"

#include "display_logic.h"

QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xAlarmQueue = NULL;
QueueHandle_t xModeQueue = NULL;
QueueSetHandle_t xDisplayEvents = NULL;
SemaphoreHandle_t serialMutex = NULL;
EventGroupHandle_t xSystemEvents = NULL;

bool rtos_objects_create(void)
{
    xDisplayQueue = xQueueCreate(1, sizeof(SensorData));
    xAlarmQueue = xQueueCreate(1, sizeof(SensorData));
    xModeQueue = xQueueCreate(1, sizeof(DisplayMode));
    serialMutex = xSemaphoreCreateMutex();
    xSystemEvents = xEventGroupCreate();
    xDisplayEvents = xQueueCreateSet(2);

    if (xDisplayQueue == NULL || xAlarmQueue == NULL || xModeQueue == NULL ||
        serialMutex == NULL || xSystemEvents == NULL || xDisplayEvents == NULL)
    {
        return false;
    }

    if (xQueueAddToSet(xDisplayQueue, xDisplayEvents) != pdPASS ||
        xQueueAddToSet(xModeQueue, xDisplayEvents) != pdPASS)
    {
        return false;
    }

    xEventGroupSetBits(xSystemEvents, EVENT_ACTIVE);
    return true;
}
