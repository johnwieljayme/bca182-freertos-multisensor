#include "alarm.h"

#include <stdio.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "alarm_logic.h"
#include "rtos_objects.h"
#include "serial_log.h"

static TIM_HandleTypeDef htim1;
static constexpr uint32_t kPwmTickHz = 1000000;
static constexpr uint32_t kBuzzerHz = 1000;
static constexpr uint32_t kRecheckMs = 500;

static const char *alarm_name(AlarmState state)
{
    switch (state)
    {
        case AlarmState::LOW_TEMPERATURE: return "LOW";
        case AlarmState::HIGH_TEMPERATURE: return "HIGH";
        case AlarmState::NORMAL: return "NORMAL";
    }
    return "NORMAL";
}

void alarm_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    bool apb2Undivided = (RCC->CFGR & RCC_CFGR_PPRE2) == RCC_CFGR_PPRE2_DIV1;
    uint32_t timerClock = apb2Undivided ? pclk2 : 2U * pclk2;
    uint32_t prescaler = timerClock / kPwmTickHz - 1U;
    uint32_t period = kPwmTickHz / kBuzzerHz - 1U;

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = prescaler;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = period;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim1);

    TIM_OC_InitTypeDef channel = {0};
    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = (period + 1U) / 2U;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    channel.OCIdleState = TIM_OCIDLESTATE_RESET;
    channel.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1, &channel, TIM_CHANNEL_1);

    char line[96];
    snprintf(line, sizeof(line), "ALARM: TIM1 clock %lu Hz, PSC=%lu, ARR=%lu -> %lu Hz PWM",
             (unsigned long)timerClock, (unsigned long)prescaler,
             (unsigned long)period,
             (unsigned long)(timerClock / (prescaler + 1U) / (period + 1U)));
    log_line(line);
}

void AlarmTask(void *pvParameters)
{
    (void)pvParameters;
    AlarmState state = AlarmState::NORMAL;
    bool sounding = false;
    log_line("AlarmTask started");

    for (;;)
    {
        SensorData sample;
        if (xQueueReceive(xAlarmQueue, &sample, pdMS_TO_TICKS(kRecheckMs)) == pdTRUE)
        {
            AlarmState next = evaluateTemperature(sample.temperature);
            if (next != state)
            {
                state = next;
                char line[48];
                snprintf(line, sizeof(line), "ALARM: temperature %s", alarm_name(state));
                log_line(line);
            }
        }

        bool active = (xEventGroupGetBits(xSystemEvents) & EVENT_ACTIVE) != 0;
        bool shouldSound = active && state != AlarmState::NORMAL;
        if (shouldSound != sounding)
        {
            sounding = shouldSound;
            if (sounding)
            {
                HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
                xEventGroupSetBits(xSystemEvents, EVENT_ALARM);
                log_line("ALARM: buzzer on");
            }
            else
            {
                HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
                xEventGroupClearBits(xSystemEvents, EVENT_ALARM);
                log_line("ALARM: buzzer off");
            }
        }
    }
}
