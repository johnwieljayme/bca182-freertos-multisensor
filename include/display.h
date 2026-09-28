#ifndef DISPLAY_H
#define DISPLAY_H

#include "FreeRTOS.h"
#include "task.h"
#include "display_logic.h"
#include "rtos_objects.h"

void display_init(void);
void DisplayTask(void *pvParameters);

#endif /* DISPLAY_H */