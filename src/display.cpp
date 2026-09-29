#include "display.h"
#include <string.h>
#include <stdio.h>

#include "alarm_logic.h"
#include "serial_log.h"

static I2C_HandleTypeDef oled_i2c;
static uint8_t SSD1306_Buffer[1024];
#define OLED_ADDR (0x3C << 1)
#define SSD1306_CTRL_CMD 0x00
#define SSD1306_CTRL_DATA 0x40

static bool oledBusReady;
static HAL_StatusTypeDef oledInitStatus;
static uint32_t oledInitError;
static HAL_StatusTypeDef oledProbeStatus;
static uint32_t oledProbeError;
static HAL_StatusTypeDef oledTransferStatus;
static uint32_t oledTransferError;

// Minimal 5x7 ASCII font (characters 32 to 127)
static const uint8_t font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62}, {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00}, {0x08,0x2A,0x1C,0x2A,0x08}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
    {0x00,0x08,0x14,0x22,0x41}, {0x14,0x14,0x14,0x14,0x14}, {0x41,0x22,0x14,0x08,0x00}, {0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x41,0x51,0x32},
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x04,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, {0x7F,0x20,0x18,0x20,0x7F},
    {0x63,0x14,0x08,0x14,0x63}, {0x03,0x04,0x78,0x04,0x03}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00}, {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78}, {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7E,0x09,0x01,0x02}, {0x08,0x14,0x54,0x54,0x3C},
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00}, {0x20,0x40,0x44,0x3D,0x00}, {0x00,0x7F,0x10,0x28,0x44},
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78}, {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C}, {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C}, {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C}, {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00}, {0x08,0x08,0x2A,0x1C,0x08}, {0x08,0x14,0x2A,0x14,0x22}
};

static HAL_StatusTypeDef WriteCmd(uint8_t cmd) {
    uint8_t data[2] = {SSD1306_CTRL_CMD, cmd};
    oledTransferStatus = HAL_I2C_Master_Transmit(&oled_i2c, OLED_ADDR, data, sizeof(data), 100);
    if (oledTransferStatus != HAL_OK) {
        oledTransferError = HAL_I2C_GetError(&oled_i2c);
    }
    return oledTransferStatus;
}

void display_init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    oled_i2c.Instance = I2C1;
    oled_i2c.Init.ClockSpeed = 100000;
    oled_i2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    oled_i2c.Init.OwnAddress1 = 0;
    oled_i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    oled_i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    oled_i2c.Init.OwnAddress2 = 0;
    oled_i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    oled_i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    oledInitStatus = HAL_I2C_Init(&oled_i2c);
    oledInitError = HAL_I2C_GetError(&oled_i2c);
    oledBusReady = oledInitStatus == HAL_OK;
}

static bool Controller_Init(void) {
    vTaskDelay(pdMS_TO_TICKS(100));

    oledProbeStatus = oledBusReady
        ? HAL_I2C_IsDeviceReady(&oled_i2c, OLED_ADDR, 3, 100)
        : HAL_ERROR;
    oledProbeError = HAL_I2C_GetError(&oled_i2c);

    char diagnostic[128];
    snprintf(diagnostic, sizeof(diagnostic),
             "DISPLAY: I2C init=%d err=0x%08lX probe=%d err=0x%08lX",
             (int)oledInitStatus, (unsigned long)oledInitError,
             (int)oledProbeStatus, (unsigned long)oledProbeError);
    log_line(diagnostic);

    if (!oledBusReady || oledProbeStatus != HAL_OK) {
        return false;
    }

    uint8_t init_seq[] = {
        0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40,
        0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3, 0x00,
        0xD5, 0xF0, 0xD9, 0x22, 0xDA, 0x12, 0xDB, 0x20, 0x8D, 0x14, 0xAF
    };
    for (size_t i = 0; i < sizeof(init_seq); i++) {
        if (WriteCmd(init_seq[i]) != HAL_OK) {
            return false;
        }
    }
    return true;
}

static bool Display_Update(void) {
    if (WriteCmd(0x21) != HAL_OK || WriteCmd(0) != HAL_OK || WriteCmd(127) != HAL_OK ||
        WriteCmd(0x22) != HAL_OK || WriteCmd(0) != HAL_OK || WriteCmd(7) != HAL_OK) {
        return false;
    }

    uint8_t chunk[129];
    chunk[0] = SSD1306_CTRL_DATA;
    for (uint16_t offset = 0; offset < sizeof(SSD1306_Buffer); offset += 128) {
        memcpy(&chunk[1], &SSD1306_Buffer[offset], 128);
        oledTransferStatus = HAL_I2C_Master_Transmit(&oled_i2c, OLED_ADDR, chunk, sizeof(chunk), 100);
        if (oledTransferStatus != HAL_OK) {
            oledTransferError = HAL_I2C_GetError(&oled_i2c);
            return false;
        }
    }
    return true;
}

static void DrawChar(int x, int y, char c) {
    if(c < 32 || c > 127) return;
    const uint8_t* b = font5x7[c - 32];
    for(int i = 0; i < 5; i++) {
        int idx = (y / 8) * 128 + x + i;
        if(idx >= 0 && idx < 1024) {
            SSD1306_Buffer[idx] |= b[i]; // Bitwise OR prevents overwriting adjacent characters
        }
    }
}

static void DrawString(int x, int y, const char* str) {
    while(*str) {
        DrawChar(x, y, *str++);
        x += 6;
    }
}

static void Draw_Tenths(char *out, size_t size, float value, const char *unit) {
    int tenths = (int)(value * 10.0f + (value >= 0.0f ? 0.5f : -0.5f));
    int magnitude = tenths < 0 ? -tenths : tenths;
    snprintf(out, size, "%s%d.%d %s", tenths < 0 ? "-" : "",
             magnitude / 10, magnitude % 10, unit);
}

static bool Draw_Screen(DisplayMode mode, const SensorData *sample,
                        bool motion, bool active, const char *alarmText) {
    memset(SSD1306_Buffer, 0, sizeof(SSD1306_Buffer));

    // Header
    DrawString(28, 0, "ROOM MONITOR");

    char value[32];

    // Show only the currently selected measurement
    DrawString(8, 16, displayModeLabel(mode));

    switch (mode) {
        case DisplayMode::TEMPERATURE:
            Draw_Tenths(value, sizeof(value), sample->temperature, "C");
            break;

        case DisplayMode::HUMIDITY:
            Draw_Tenths(value, sizeof(value), sample->humidity, "%");
            break;

        case DisplayMode::LIGHT:
            snprintf(value, sizeof(value), "%d %%", sample->lightLevel);
            break;

        case DisplayMode::MOTION:
            snprintf(value, sizeof(value),
                     "%s", motion ? "DETECTED" : "CLEAR");
            break;
    }

    // Selected measurement value
    DrawString(8, 28, value);

    // System status
    DrawString(8, 44, active ? "STATE ACTIVE" : "STATE INACTIVE");

    // Alarm status
    DrawString(8, 56, alarmText != NULL ? alarmText : "ALARM NORMAL");

    return Display_Update();
}

static bool Clear_Screen(void) {
    memset(SSD1306_Buffer, 0, sizeof(SSD1306_Buffer));
    return Display_Update();
}

void DisplayTask(void *pvParameters) {
    (void)pvParameters;
    DisplayMode mode = DisplayMode::TEMPERATURE;
    SensorData sample = {0.0f, 0.0f, 0, false};
    bool haveSample = false;
    bool active = true;
    bool motion = false;
    bool alarm = false;
    bool blanked = false;

    log_line("DisplayTask started");
    bool displayReady = Controller_Init();
    log_line(displayReady ? "DISPLAY: OLED initialised" :
                            "DISPLAY: OLED I2C init failed (check PB6/PB7 and 0x3C)");
    bool writeErrorLogged = false;

    for (;;) {
        QueueSetMemberHandle_t ready =
            xQueueSelectFromSet(xDisplayEvents, pdMS_TO_TICKS(250));
        bool changed = false;

        if (ready == xDisplayQueue && xQueueReceive(xDisplayQueue, &sample, 0) == pdTRUE) {
            haveSample = true;
            changed = true;
        } else if (ready == xModeQueue && xQueueReceive(xModeQueue, &mode, 0) == pdTRUE) {
            changed = true;
        }

        EventBits_t bits = xEventGroupGetBits(xSystemEvents);
        bool nowActive = (bits & EVENT_ACTIVE) != 0;
        bool nowMotion = (bits & EVENT_MOTION) != 0;
        bool nowAlarm = (bits & EVENT_ALARM) != 0;
        changed = changed || nowActive != active || nowMotion != motion || nowAlarm != alarm;
        active = nowActive;
        motion = nowMotion;
        alarm = nowAlarm;

        if (!active) {
            if (changed && !blanked) {
                if (!Clear_Screen() && !writeErrorLogged) {
                    char diagnostic[96];
                    snprintf(diagnostic, sizeof(diagnostic),
                             "DISPLAY: OLED write failed status=%d err=0x%08lX",
                             (int)oledTransferStatus, (unsigned long)oledTransferError);
                    log_line(diagnostic);
                    writeErrorLogged = true;
                }
                blanked = true;
            }
            continue;
        }
        blanked = false;

        if (haveSample && changed) {
            const char *alarmText = NULL;
            if (alarm) {
                alarmText = evaluateTemperature(sample.temperature) == AlarmState::LOW_TEMPERATURE
                                ? "ALARM LOW" : "ALARM HIGH";
            }
            if (!Draw_Screen(mode, &sample, motion, active, alarmText) && !writeErrorLogged) {
                char diagnostic[96];
                snprintf(diagnostic, sizeof(diagnostic),
                         "DISPLAY: OLED write failed status=%d err=0x%08lX",
                         (int)oledTransferStatus, (unsigned long)oledTransferError);
                log_line(diagnostic);
                writeErrorLogged = true;
            }
        }
    }
}