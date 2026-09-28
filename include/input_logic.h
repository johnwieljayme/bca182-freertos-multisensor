#ifndef INPUT_LOGIC_H
#define INPUT_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

int8_t encoder_step(bool previousClockHigh, bool clockHigh, bool dataHigh);
bool encoder_button_pressed(bool switchHigh);

#endif
