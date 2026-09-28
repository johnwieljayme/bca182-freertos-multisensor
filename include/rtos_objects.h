#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include <stdbool.h>

// Part V, Number 24: Define Sensor Data
struct SensorData {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
};

constexpr EventBits_t EVENT_ACTIVE = (1U << 0);
constexpr EventBits_t EVENT_MOTION = (1U << 1);
constexpr EventBits_t EVENT_ALARM = (1U << 2);

extern QueueHandle_t xDisplayQueue;
extern QueueHandle_t xAlarmQueue;
extern QueueHandle_t xModeQueue;
extern QueueSetHandle_t xDisplayEvents;
extern SemaphoreHandle_t serialMutex;
extern EventGroupHandle_t xSystemEvents;

bool rtos_objects_create(void);

#endif /* RTOS_OBJECTS_H */