#include "input.h"

#include <stdio.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "display_logic.h"
#include "input_logic.h"
#include "rtos_objects.h"
#include "serial_log.h"

static constexpr uint32_t kInputPeriodMs = 5;

void input_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void InputTask(void *pvParameters)
{
    (void)pvParameters;
    bool previousClock = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET;
    bool previousPressed = false;
    DisplayMode mode = DisplayMode::TEMPERATURE;
    char line[40];

    log_line("InputTask started");
    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kInputPeriodMs));
        bool clockHigh = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET;
        bool dataHigh = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET;
        int8_t step = encoder_step(previousClock, clockHigh, dataHigh);
        previousClock = clockHigh;

        bool active = (xEventGroupGetBits(xSystemEvents) & EVENT_ACTIVE) != 0;
        if (active && step != 0)
        {
            mode = step > 0 ? nextDisplayMode(mode) : previousDisplayMode(mode);
            xQueueOverwrite(xModeQueue, &mode);
            snprintf(line, sizeof(line), "INPUT: mode %s", displayModeLabel(mode));
            log_line(line);
        }

        bool switchHigh = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET;
        bool pressed = encoder_button_pressed(switchHigh);
        if (active && pressed && !previousPressed)
        {
            log_line("INPUT: button pressed");
        }
        previousPressed = pressed;
    }
}
