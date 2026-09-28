#ifndef ALARM_LOGIC_H
#define ALARM_LOGIC_H

#include <stdint.h>

enum class AlarmState : uint8_t
{
    NORMAL,
    LOW_TEMPERATURE,
    HIGH_TEMPERATURE
};

AlarmState evaluateTemperature(float temperature);

#endif
