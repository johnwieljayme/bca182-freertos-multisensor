#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "FreeRTOS.h"
#include "queue.h"
#include <stdbool.h>

// Part V, Number 24: Define Sensor Data
struct SensorData {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
};

// Global handle for the sensor queue
extern QueueHandle_t xSensorQueue;

#endif /* RTOS_OBJECTS_H */