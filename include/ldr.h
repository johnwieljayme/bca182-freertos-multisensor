#ifndef LDR_H
#define LDR_H

#include "stm32f1xx_hal.h"

void LDR_Init(void);
float LDR_Read_Percent(void);

#endif /* LDR_H */