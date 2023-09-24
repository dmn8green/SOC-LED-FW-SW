//******************************************************************************
/**
 * @file StaticAnimation.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief StaticAnimation class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#pragma once

#include "BaseAnimation.h"

//******************************************************************************
/**
 * @brief Static color animation
 * 
 * This animation sets all of the LED pixels to the same color.
 * 
 * The color is set using the reset() method.
 * 
 * Since it is static, the rate is set to portMAX_DELAY (in the base class).
 * 
 * The refresh will get called only once.  If the led is unplugged or loose
 * power, it will not be refreshed again until the rate is changed or a refresh
 * is forced.  This is a scenario that should never happen.
 */
class StaticAnimation : public BaseAnimation {
public:
    void reset(uint32_t color);
    void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) override;

private:
    uint32_t color;
};
