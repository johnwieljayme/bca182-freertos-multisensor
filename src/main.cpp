#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "alarm.h"
#include "display.h"
#include "input.h"
#include "motion.h"
#include "rtos_objects.h"
#include "sensors.h"
#include "serial_log.h"
#include "system_state.h"

extern "C" uint32_t g_pfnVectors[];

static void SystemClock_Config(void);
void app_main(void);

int main(void)
{
    SCB->VTOR = (uint32_t)g_pfnVectors;
    HAL_Init();
    SystemClock_Config();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    app_main();
    for (;;) {}
}

void app_main(void)
{
    serial_log_init();
    log_line("BCA182 FreeRTOS Multisensor");
    log_line("System starting...");

    sensors_init();
    motion_init();
    input_init();
    display_init();
    alarm_init();

    if (!rtos_objects_create())
    {
        log_line("FATAL: could not create RTOS objects (FreeRTOS heap)");
        for (;;) {}
    }

    BaseType_t created =
        xTaskCreate(SensorTask, "SensorTask", 256, NULL, 2, NULL) == pdPASS &&
        xTaskCreate(DisplayTask, "DisplayTask", 256, NULL, 1, NULL) == pdPASS &&
        xTaskCreate(InputTask, "InputTask", 128, NULL, 3, NULL) == pdPASS &&
        xTaskCreate(MotionTask, "MotionTask", 128, NULL, 3, NULL) == pdPASS &&
        xTaskCreate(AlarmTask, "AlarmTask", 128, NULL, 2, NULL) == pdPASS &&
        xTaskCreate(StateTask, "StateTask", 128, NULL, 2, NULL) == pdPASS;

    if (!created)
    {
        log_line("FATAL: could not create all tasks (FreeRTOS heap)");
        for (;;) {}
    }

    vTaskStartScheduler();
    log_line("FATAL: scheduler did not start (FreeRTOS heap)");
    for (;;) {}
}

extern "C" HAL_StatusTypeDef HAL_InitTick(uint32_t tickPriority)
{
    __HAL_RCC_TIM4_CLK_ENABLE();
    TIM4->PSC = 7;
    TIM4->ARR = 999;
    TIM4->CNT = 0;
    TIM4->DIER |= TIM_DIER_UIE;
    TIM4->CR1 |= TIM_CR1_CEN;

    HAL_NVIC_SetPriority(TIM4_IRQn, tickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    return HAL_OK;
}

extern "C" void TIM4_IRQHandler(void)
{
    if ((TIM4->SR & TIM_SR_UIF) != 0)
    {
        TIM4->SR &= ~TIM_SR_UIF;
        HAL_IncTick();
    }
}

extern "C" void vAssertCalled(const char *file, int line)
{
    (void)file;
    (void)line;
    __disable_irq();
    for (;;) {}
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t task, char *taskName)
{
    (void)task;
    (void)taskName;
    __disable_irq();
    for (;;) {}
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clock = {0};

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscillator.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&oscillator);

    clock.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clock.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clock.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clock.APB1CLKDivider = RCC_HCLK_DIV1;
    clock.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_0);
}
