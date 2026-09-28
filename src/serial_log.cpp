#include "serial_log.h"

#include <string.h>

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_objects.h"

static UART_HandleTypeDef huart1;

void serial_log_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void log_line(const char *message)
{
    bool locked = serialMutex != NULL &&
                  xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
    if (locked)
    {
        xSemaphoreTake(serialMutex, portMAX_DELAY);
    }

    static const uint8_t newline[] = "\r\n";
    HAL_UART_Transmit(&huart1, (const uint8_t *)message,
                      (uint16_t)strlen(message), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, newline, sizeof(newline) - 1, HAL_MAX_DELAY);

    if (locked)
    {
        xSemaphoreGive(serialMutex);
    }
}
