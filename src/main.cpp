#include "stm32f1xx_hal.h"
#include <cstring>

UART_HandleTypeDef huart1;

static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void UART_Print(const char *message);

int main(void)
{
    // Initialize the HAL library (this configures SysTick for the default 8MHz clock)
    HAL_Init();

    // PERMANENTLY REMOVED SystemClock_Config() to prevent Wokwi simulator timeouts

    MX_GPIO_Init();
    MX_USART1_UART_Init();

    UART_Print("BCA182 FreeRTOS Multisensor\r\nSystem starting...\r\n");

    while (true)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        UART_Print("Wokwi UART heartbeat\r\n");
        
        // Software delay
        for (volatile uint32_t i = 0; i < 500000; i++) 
        {
        }
    }
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {};
    
    // PA9 is TX
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA10 is RX
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        while (true) {}
    }
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {};

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void UART_Print(const char *message)
{
    HAL_UART_Transmit(
        &huart1, 
        reinterpret_cast<const uint8_t *>(message), 
        static_cast<uint16_t>(std::strlen(message)), 
        HAL_MAX_DELAY
    );
}

extern "C" {
    void SysTick_Handler(void)
    {
        HAL_IncTick();
    }
}