#include "dht22.h"

// Resolved GCC 7.2.1 constexpr strictness by using a const pointer
GPIO_TypeDef * const DHT_PORT = GPIOB;
const uint16_t DHT_PIN = GPIO_PIN_0;

static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void Delay_us(uint32_t us)
{
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);
    while (DWT->CYCCNT - startTick < delayTicks) {}
}

static void Set_Pin_Output(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void Set_Pin_Input(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; 
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void DHT22_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    DWT_Init();
    Set_Pin_Output(DHT_PORT, DHT_PIN);
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
}

DHT22_Data_t DHT22_Read(void)
{
    DHT22_Data_t data = {0.0f, 0.0f, false};
    uint8_t buffer[5] = {0};
    
    Set_Pin_Output(DHT_PORT, DHT_PIN);
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_RESET);
    Delay_us(1200); 
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
    Delay_us(30);
    
    Set_Pin_Input(DHT_PORT, DHT_PIN);
    
    uint32_t timeout = 0;
    while(HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) == GPIO_PIN_SET)
    {
        if(++timeout > 100) return data;
        Delay_us(1);
    }
    
    timeout = 0;
    while(HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) == GPIO_PIN_RESET)
    {
        if(++timeout > 100) return data;
        Delay_us(1);
    }
    
    timeout = 0;
    while(HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) == GPIO_PIN_SET)
    {
        if(++timeout > 100) return data;
        Delay_us(1);
    }
    
    for(int i = 0; i < 40; i++)
    {
        timeout = 0;
        while(HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) == GPIO_PIN_RESET)
        {
            if(++timeout > 100) return data;
            Delay_us(1);
        }
        
        uint32_t tStart = DWT->CYCCNT;
        timeout = 0;
        while(HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) == GPIO_PIN_SET)
        {
            if(++timeout > 100) return data;
            Delay_us(1);
        }
        uint32_t tEnd = DWT->CYCCNT;
        uint32_t pulseLength = (tEnd - tStart) / (SystemCoreClock / 1000000);
        
        int byteIdx = i / 8;
        buffer[byteIdx] <<= 1;
        
        if (pulseLength > 40)
        {
            buffer[byteIdx] |= 1;
        }
    }
    
    uint8_t sum = buffer[0] + buffer[1] + buffer[2] + buffer[3];
    if (sum == buffer[4])
    {
        uint16_t rawHumidity = (buffer[0] << 8) | buffer[1];
        uint16_t rawTemp = (buffer[2] << 8) | buffer[3];
        
        data.humidity = (float)rawHumidity / 10.0f;
        
        if (rawTemp & 0x8000)
        {
            data.temperature = (float)(rawTemp & 0x7FFF) / -10.0f;
        }
        else
        {
            data.temperature = (float)rawTemp / 10.0f;
        }
        data.valid = true;
    }
    
    return data;
}