#include "stm32f1xx_hal.h"

void app_main()
{
    // Application entry point.
    // FreeRTOS objects and tasks will be added in later parts.
}

int main(void)
{
    HAL_Init();

    app_main();

    while (true)
    {
        // Temporary idle loop.
        // This will be replaced by the FreeRTOS scheduler-driven
        // application in the next stages.
    }
}