#include "input_logic.h"

int8_t encoder_step(bool previousClockHigh, bool clockHigh, bool dataHigh)
{
    if (previousClockHigh && !clockHigh)
    {
        return dataHigh != clockHigh ? 1 : -1;
    }
    return 0;
}

bool encoder_button_pressed(bool switchHigh)
{
    return !switchHigh;
}
