//******************************************************************************
/**
 * @file  ChargeIndicator.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 *         bill mcguinness (bmcguinness@appliedlogix.com)
 * @brief ChargeIndicator class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 */
//******************************************************************************

#include <algorithm>
#include "ChargeIndicator.h"

#include "esp_log.h"

//static const char* TAG = "CI"; // Charge Indicator

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 * 
 * @param led_pixels   Pointer to the LED pixels
 * @param start_pixel  Starting pixel
 * @param led_count    Number of LED pixels to be updated
 *
 * @returns Number of LEDs/pixels updated. May NOT be the same as 'led_count' if
 *          animation is in progress.
 *
 * Interesting values of led_count must be handled:
 *    0:  progress_animation must be called in case latent animation is going
 *    1:  May have to render one of the two CL LEDs.
 *
 */
int ChargeIndicator::refresh(uint8_t* led_pixels, int start_pixel, int led_count) {

    led_count = std::max (0, led_count);

    int animation_leds = std::max (0, led_count - (int)ci_led_count);
    int leds_updated = 0;

    leds_updated += progress_animation.refresh (led_pixels, start_pixel, animation_leds);
    bool latentAnimation = (leds_updated == animation_leds) ? false:true;

    if (latentAnimation)
    {
        leds_updated += charge_level.refresh (led_pixels, start_pixel + leds_updated, ci_led_count);
    }
    else
    {
        leds_updated += charge_level.refresh (led_pixels, start_pixel + leds_updated, (led_count - animation_leds));
    }

    return leds_updated;
}

//******************************************************************************
/**
 * @brief Reset the state of the charge indicator 
 * 
 * @param[in] color_fg      Color to set the "charging" LED pixels (foreground)
 * @param[in] color_bg      Color to set the "non-charging" LED pixels (background)
 * @param[in] chase_color   Color for progress (chase) indicator (usually same as color_fg)  
 * @param[in] ci_led_count  Number of LEDs for the fixed "charge level indicator" just above 
 *                          animations.  
 */
void ChargeIndicator::reset(uint32_t color_fg, uint32_t color_bg,
                            uint32_t chase_color, uint32_t ci_led_count) {

    this->color_fg = color_fg;
    this->color_bg = color_bg;
    this->chase_color = chase_color;
    this->fill_level = 0;
    this->ci_led_count = ci_led_count;

    progress_animation.reset(chase_color, color_bg);
    charge_level.reset(color_fg);

    this->BaseAnimation::reset();
}
