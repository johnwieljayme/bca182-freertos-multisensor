#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "dht22.h"
#include "ldr.h"
#include "rtos_objects.h"

UART_HandleTypeDef huart1;
extern "C" uint32_t g_pfnVectors[];

// Instantiate the global queue handle
QueueHandle_t xSensorQueue = NULL;

void SystemClock_Config(void);
static void MX_USART1_UART_Init(void);
static void Heartbeat_GPIO_Init(void);

static void UartPrint(const char* s)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void UartPrintUInt(const char* label, unsigned long val)
{
    char digits[12];
    int di = 0;
    if (val == 0) { digits[di++] = '0'; }
    while (val > 0) { digits[di++] = '0' + (val % 10); val /= 10; }
    UartPrint(label);
    char out[16];
    int oi = 0;
    while (di > 0) { out[oi++] = digits[--di]; }
    out[oi++] = '\r';
    out[oi++] = '\n';
    out[oi] = '\0';
    UartPrint(out);
}

static void UartPrintFloat(const char* label, float val, const char* unit)
{
    UartPrint(label);
    if (val < 0.0f) {
        UartPrint("-");
        val = -val;
    }
    
    int whole = (int)val;
    int decimal = (int)((val - whole) * 100.0f + 0.5f);
    
    char digits[12];
    int di = 0;
    if (whole == 0) { digits[di++] = '0'; }
    while (whole > 0) { digits[di++] = '0' + (whole % 10); whole /= 10; }
    
    char out[16];
    int oi = 0;
    while (di > 0) { out[oi++] = digits[--di]; }
    out[oi++] = '.';
    out[oi++] = '0' + (decimal / 10);
    out[oi++] = '0' + (decimal % 10);
    out[oi] = '\0';
    
    UartPrint(out);
    UartPrint(unit);
    UartPrint("\r\n");
}

static void FaultBlink_Forever(int pulsesPerCycle)
{
    for (;;)
    {
        for (int i = 0; i < pulsesPerCycle; i++)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 100000; d++) {}
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            for (volatile int d = 0; d < 100000; d++) {}
        }
        for (volatile int d = 0; d < 800000; d++) {}
    }
}

extern "C" void HardFault_Handler(void) __attribute__((naked));
extern "C" void HardFault_Handler(void)
{
    __asm volatile
    (
        " tst lr, #4                \n"
        " ite eq                    \n"
        " mrseq r0, msp             \n"
        " mrsne r0, psp             \n"
        " b hard_fault_handler_c    \n"
    );
}

extern "C" void hard_fault_handler_c(unsigned long *stack)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    unsigned long pc = stack[6];
    unsigned long lr = stack[5];
    unsigned long cfsr = *(volatile unsigned long*)0xE000ED28;
    unsigned long hfsr = *(volatile unsigned long*)0xE000ED2C;
    (void)pc; (void)lr;

    uint8_t mmfsr = (uint8_t)(cfsr & 0xFF);
    uint8_t bfsr  = (uint8_t)((cfsr >> 8) & 0xFF);
    uint16_t ufsr = (uint16_t)((cfsr >> 16) & 0xFFFF);

    int blinkCode;
    if (ufsr != 0)      blinkCode = 3;
    else if (bfsr != 0) blinkCode = 2;
    else if (mmfsr != 0) blinkCode = 1;
    else                 blinkCode = 5;

    for (;;)
    {
        for (int i = 0; i < blinkCode; i++)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 300000; d++) {}
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            for (volatile int d = 0; d < 300000; d++) {}
        }
        for (volatile int d = 0; d < 2000000; d++) {}
    }
}

extern "C" void vAssertCalled(const char* file, int line)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    UartPrint("\r\n[ASSERT FAILED]\r\n");
    for (;;) {}
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    UartPrint("\r\n[ERROR: STACK OVERFLOW DETECTED!]\r\n");
    FaultBlink_Forever(6);
}

extern "C" void vApplicationIdleHook(void)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    for (volatile int i = 0; i < 200000; i++) {}
}

extern "C" HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    __HAL_RCC_TIM4_CLK_ENABLE();
    TIM4->PSC = 7;
    TIM4->ARR = 999;
    TIM4->CNT = 0;
    TIM4->DIER |= TIM_DIER_UIE;
    TIM4->CR1 |= TIM_CR1_CEN;
    HAL_NVIC_SetPriority(TIM4_IRQn, TickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    return HAL_OK;
}

extern "C" void TIM4_IRQHandler(void)
{
    if (TIM4->SR & TIM_SR_UIF)
    {
        TIM4->SR &= ~TIM_SR_UIF;
        HAL_IncTick();
    }
}

void SensorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    for (;;)
    {
        struct SensorData data = {0};
        
        DHT22_Data_t dhtData = DHT22_Read();
        if (dhtData.valid)
        {
            data.temperature = dhtData.temperature;
            data.humidity = dhtData.humidity;
        }

        // LDR percent is cast to an int to match the lab struct specification
        data.lightLevel = (int)LDR_Read_Percent();
        data.motionDetected = false; // PIR not yet implemented
        
        UartPrintUInt("\r\nSensorTask posting data @tick ", (unsigned long)xTaskGetTickCount());
        
        // Send the struct to the queue. Wait time is 0 (do not block if full).
        xQueueSend(xSensorQueue, &data, 0);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
}

void TaskB(void *pvParameters)
{
    (void)pvParameters;
    struct SensorData receivedData;
    
    for (;;)
    {
        // Block indefinitely (portMAX_DELAY) until SensorTask pushes data into the queue
        if (xQueueReceive(xSensorQueue, &receivedData, portMAX_DELAY) == pdPASS)
        {
            UartPrintUInt("TaskB received data @tick ", (unsigned long)xTaskGetTickCount());
            UartPrintFloat("Temperature: ", receivedData.temperature, " C");
            UartPrintFloat("Humidity: ", receivedData.humidity, " %");
            UartPrintUInt("Light Level (%): ", (unsigned long)receivedData.lightLevel);
        }
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    
    SCB->VTOR = (uint32_t)g_pfnVectors;
    __DSB();
    __ISB();

    MX_USART1_UART_Init();
    Heartbeat_GPIO_Init();
    DHT22_Init();
    LDR_Init();

    UartPrint("BCA182 FreeRTOS Multisensor\r\nSystem starting...\r\n");

    // Initialize the queue to hold up to 5 SensorData structs
    xSensorQueue = xQueueCreate(5, sizeof(struct SensorData));
    configASSERT(xSensorQueue != NULL);

    BaseType_t okSensor = xTaskCreate(SensorTask, "SensorTask", 256, NULL, 2, NULL);
    BaseType_t okB = xTaskCreate(TaskB, "TaskB", 256, NULL, 1, NULL);

    (void)okSensor;
    (void)okB;

    vTaskStartScheduler();
    UartPrint("!!! Scheduler start FAILED (returned) !!!\r\n");
    while (1) {}
}

static void Heartbeat_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gi = {0};
    gi.Pin = GPIO_PIN_13;
    gi.Mode = GPIO_MODE_OUTPUT_PP;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gi);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}