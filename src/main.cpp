#include "stm32f1xx_hal.h"

#include <stdio.h>

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
    SystemCoreClockUpdate();
    log_line("BCA182 FreeRTOS Multisensor");
    log_line("System starting...");

    char clockLine[96];
    snprintf(clockLine, sizeof(clockLine),
             "CLOCK: core=%lu HCLK=%lu PCLK1=%lu PCLK2=%lu Hz",
             (unsigned long)SystemCoreClock,
             (unsigned long)HAL_RCC_GetHCLKFreq(),
             (unsigned long)HAL_RCC_GetPCLK1Freq(),
             (unsigned long)HAL_RCC_GetPCLK2Freq());
    log_line(clockLine);

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
        xTaskCreate(AlarmTask, "AlarmTask", 256, NULL, 2, NULL) == pdPASS &&
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
    /* Match Wokwi's reset clock and the reference project: HSI at 8 MHz. */
}
