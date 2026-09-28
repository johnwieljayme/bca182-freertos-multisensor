#ifndef DISPLAY_LOGIC_H
#define DISPLAY_LOGIC_H

#include <stdint.h>

enum class DisplayMode : uint8_t
{
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
};

DisplayMode nextDisplayMode(DisplayMode mode);
DisplayMode previousDisplayMode(DisplayMode mode);
const char *displayModeLabel(DisplayMode mode);

#endif
