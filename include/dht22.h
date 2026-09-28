#ifndef DHT22_H
#define DHT22_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>

typedef struct {
    float temperature;
    float humidity;
    bool valid;
} DHT22_Data_t;

void DHT22_Init(void);
DHT22_Data_t DHT22_Read(void);

#endif /* DHT22_H */