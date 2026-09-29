#include "sensors.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dht22.h"
#include "ldr.h"
#include "rtos_objects.h"
#include "serial_log.h"

static constexpr uint32_t kSensorPeriodMs = 2000;

void sensors_init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    DHT22_Init();
    LDR_Init();
}

void SensorTask(void *pvParameters)
{
    (void)pvParameters;
    log_line("SensorTask started");

    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(kSensorPeriodMs));
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        if ((xEventGroupGetBits(xSystemEvents) & EVENT_ACTIVE) == 0)
        {
            continue;
        }

        DHT22_Data_t dht;
        taskENTER_CRITICAL();
        dht = DHT22_Read();
        taskEXIT_CRITICAL();

        if (!dht.valid)
        {
            log_line("DHT22: read failed (checksum or timeout), sample skipped");
            continue;
        }

        SensorData sample = {0};
        sample.temperature = dht.temperature;
        sample.humidity = dht.humidity;
        sample.lightLevel = (int)(LDR_Read_Percent() + 0.5f);
        sample.motionDetected =
            (xEventGroupGetBits(xSystemEvents) & EVENT_MOTION) != 0;

        xQueueOverwrite(xDisplayQueue, &sample);
        xQueueOverwrite(xAlarmQueue, &sample);
    }
}
