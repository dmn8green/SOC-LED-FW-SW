//*****************************************************************************
/**
 * @file Colors.h
 * @author bill mcguinness (bmcguinness@appliedlogix.com)
 * @brief Color conversion
 * @version 0.1
 * @date 2023-08-25
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//*****************************************************************************

#pragma once

#include "Utils/Singleton.h"

#include <stdint.h>

typedef enum
{
    LED_COLOR_UNKNOWN = -1,
    LED_COLOR_START = 1000,
    LED_COLOR_RED = LED_COLOR_START,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_WHITE,
    LED_COLOR_BLACK,
    LED_COLOR_ORANGE,
    LED_COLOR_CYAN,
    LED_COLOR_YELLOW,
    LED_COLOR_PURPLE,
    LED_COLOR_DEBUG_ON,
    LED_COLOR_DEBUG_OFF,
    LED_COLOR_END
} LED_COLOR;

typedef enum
{
    LED_INTENSITY_LOW = 0,
    LED_INTENSITY_HIGH
} LED_INTENSITY;

typedef struct
{
    uint8_t h;
    uint8_t s;
    uint8_t v;
} COLOR_HSV;

/**
 * @brief Color class.
 *
 */
class Colors : public Singleton<Colors> {

	public:
    Colors (token) {};
    ~Colors (void) = default;

public:
    uint32_t    	getRgb (LED_COLOR);   // gets RGB value
    COLOR_HSV   	*getHsv (LED_COLOR);  // get HSV struct

    LED_COLOR   	isRgb (uint32_t rgb); // finds RGB value, if known
    LED_COLOR   	isHsv (COLOR_HSV *);  // finds HSV value, if known

    uint32_t    	setMode (LED_INTENSITY newMode);
    LED_INTENSITY	getMode (void);

    void 			hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);


private:

    LED_INTENSITY ledIntensity = LED_INTENSITY_HIGH;


};  // class Color
