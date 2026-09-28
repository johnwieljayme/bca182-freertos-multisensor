#include "motion.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_objects.h"
#include "serial_log.h"

static constexpr uint32_t kMotionPeriodMs = 100;

void motion_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void MotionTask(void *pvParameters)
{
    (void)pvParameters;
    bool previous = false;
    log_line("MotionTask started");

    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kMotionPeriodMs));
        bool detected = pir_motion_detected(
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET);

        if (detected)
        {
            xEventGroupSetBits(xSystemEvents, EVENT_MOTION);
        }
        else
        {
            xEventGroupClearBits(xSystemEvents, EVENT_MOTION);
        }

        if (detected != previous)
        {
            log_line(detected ? "MOTION: detected" : "MOTION: clear");
            previous = detected;
        }
    }
}
