//******************************************************************************
/**
 * @file Colors.cpp
 * @author bill mcguinness (bmcguinness@appliedlogix.com)
 * @brief Color definitions
 * @version 0.1
 * @date 2023-09-19
 *
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#include <stdint.h>
#include <map>

#include "Colors.h"

#include "esp_log.h"

#define COLOR_RED_HIGH    0xFF0000  // RGB
#define COLOR_GREEN_HIGH  0x00FF00
#define COLOR_BLUE_HIGH   0x0000FF
#define COLOR_WHITE_HIGH  0x808080
#define COLOR_ORANGE_HIGH 0xFF7000
#define COLOR_CYAN_HIGH   0x00FF80
#define COLOR_YELLOW_HIGH 0xFFFF00
#define COLOR_PURPLE_HIGH 0xFF3DA5
#define COLOR_BLACK_HIGH  0x000000
#define COLOR_DEBUG_ON_HIGH    0xFF00FF
#define COLOR_DEBUG_OFF_HIGH   0x000000


#define COLOR_RED_LOW     0x7F0000  // RGB LOW
#define COLOR_GREEN_LOW   0x007F00
#define COLOR_BLUE_LOW    0x00007F
#define COLOR_WHITE_LOW   0x404040
#define COLOR_ORANGE_LOW  0x7F3000
#define COLOR_CYAN_LOW    0x007404
#define COLOR_YELLOW_LOW  0x7F7F00
#define COLOR_PURPLE_LOW  0x7F3D65
#define COLOR_BLACK_LOW   0x000000
#define COLOR_DEBUG_ON_HIGH    0x7F007F
#define COLOR_DEBUG_OFF_HIGH   0x000000

#define COLOR_YELLOW_HSV_HIGH    60, 100,  100
#define COLOR_BLUE_HSV_HIGH      240, 100, 100
#define COLOR_WHITE_HSV_HIGH     0,   0,   100

#define COLOR_YELLOW_HSV_LOW     60, 100,  50
#define COLOR_BLUE_HSV_LOW       240, 100, 50
#define COLOR_WHITE_HSV_LOW      0,   0,   50

static std::map<uint32_t, COLOR_HSV> DayColorsHsv =
{
    { LED_COLOR_YELLOW, {COLOR_YELLOW_HSV_HIGH}},
    { LED_COLOR_BLUE,   {COLOR_BLUE_HSV_HIGH}},
    { LED_COLOR_WHITE,  {COLOR_WHITE_HSV_HIGH}}
};

static std::map<uint32_t, COLOR_HSV> NightColorsHsv =
{
    { LED_COLOR_YELLOW, {COLOR_YELLOW_HSV_LOW}},
    { LED_COLOR_BLUE,   {COLOR_BLUE_HSV_LOW}},
    { LED_COLOR_WHITE,  {COLOR_WHITE_HSV_LOW}}
};

static std::map<uint32_t, uint32_t> DayColors = {
    { LED_COLOR_RED,    COLOR_RED_HIGH},
    { LED_COLOR_GREEN,  COLOR_GREEN_HIGH},
    { LED_COLOR_BLUE,   COLOR_BLUE_HIGH},
    { LED_COLOR_WHITE,  COLOR_WHITE_HIGH},
    { LED_COLOR_ORANGE, COLOR_ORANGE_HIGH},
    { LED_COLOR_CYAN,   COLOR_CYAN_HIGH},
    { LED_COLOR_YELLOW, COLOR_YELLOW_HIGH},
    { LED_COLOR_PURPLE, COLOR_PURPLE_HIGH},
    { LED_COLOR_BLACK,  COLOR_BLACK_HIGH},
    { LED_COLOR_DEBUG_ON,  COLOR_DEBUG_ON_HIGH},
    { LED_COLOR_DEBUG_OFF, COLOR_DEBUG_OFF_HIGH}
};

static std::map<uint32_t, uint32_t> NightColors = {
    { LED_COLOR_RED,    COLOR_RED_LOW},
    { LED_COLOR_GREEN,  COLOR_GREEN_LOW},
    { LED_COLOR_BLUE,   COLOR_BLUE_LOW},
    { LED_COLOR_WHITE,  COLOR_WHITE_LOW},
    { LED_COLOR_ORANGE, COLOR_ORANGE_LOW},
    { LED_COLOR_CYAN,   COLOR_CYAN_LOW},
    { LED_COLOR_YELLOW, COLOR_YELLOW_LOW},
    { LED_COLOR_PURPLE, COLOR_PURPLE_LOW},
    { LED_COLOR_BLACK,  COLOR_BLACK_LOW},
    { LED_COLOR_DEBUG_ON,  COLOR_DEBUG_ON_HIGH},
    { LED_COLOR_DEBUG_OFF, COLOR_DEBUG_OFF_HIGH}
};

static const char *TAG = "Colors";

//******************************************************************************
/**
 * @brief Get HSV color
 */
COLOR_HSV* Colors::getHsv (LED_COLOR ledColor)
{
    COLOR_HSV *pRetVal = nullptr;

    if (ledIntensity == LED_INTENSITY_LOW)
    {
        pRetVal = &NightColorsHsv[ledColor];
    }
    else
    {
        pRetVal = &DayColorsHsv[ledColor];
    }

    return pRetVal;
}

//******************************************************************************
/**
 * @brief Set Day/Night mode
 */
uint32_t Colors::setMode (LED_INTENSITY newIntensity)
{
    ledIntensity = newIntensity;

    return 0;
}

//******************************************************************************
/**
 * @brief Get Day/Night mode
 */
LED_INTENSITY Colors::getMode (void)
{
	return ledIntensity;
}

//******************************************************************************
/**
 * @brief Get RGB color
 */
uint32_t Colors::getRgb (LED_COLOR ledColor)
{
    uint32_t rgbColor = 0;

    ESP_LOGI(TAG, "getRgb: ledColor = %d intensity %d", ledColor, ledIntensity);
    if (ledIntensity == LED_INTENSITY_LOW)
    {
        rgbColor = NightColors[ledColor];
    }
    else
    {
        rgbColor = DayColors[ledColor];
    }
    ESP_LOGI(TAG, "getRgb: rgbColor = %ld", rgbColor);

    return rgbColor;
}

//******************************************************************************
/**
 * @brief Find LED enumeration value from HSV value
 */
LED_COLOR Colors::isHsv (COLOR_HSV *pHsv)
{
    if (pHsv != nullptr)
    {
        for (auto &i : DayColorsHsv)
        {
            if (i.second.h == pHsv->h &&
                i.second.s == pHsv->s &&
                i.second.v == pHsv->v)
            { 
                return ((LED_COLOR)i.first); 
            }
        }
    }
    return LED_COLOR_UNKNOWN; 
}

//******************************************************************************
/**
 * @brief  Find LED enumeration color from raw RGB value  
 */
LED_COLOR Colors::isRgb (uint32_t rgbColor)
{
    for (auto &i : DayColors)
    {
        if (i.second == rgbColor)
        {
            return ((LED_COLOR)i.first); 
        }
    }

    for (auto &i : NightColors)
    {
        if (i.second == rgbColor)
        {
           return ((LED_COLOR)i.first);
        }
    }

    return LED_COLOR_UNKNOWN;
}

void Colors::hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    //uint32_t rgb_max = uint32_t ((v * 2.55f) + 0.5001);
    uint32_t rgb_max = (uint32_t) (((v * 255.0f)/ 100) + 0.5);
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

