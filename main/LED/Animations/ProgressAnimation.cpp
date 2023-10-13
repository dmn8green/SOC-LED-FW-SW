//******************************************************************************
/**
 * @file ProgressAnimation.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 *         bill mcguinness (bmcguinness@appliedlogix.com)
 * @brief ProgressAnimation class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * 		This class handles the moving/animated motion of the "low" and "high"
 * 		static segments. An important but subtle requirement is that we cannot
 * 		change (usually upwards) the change level until animation has completed.
 * 		The visual effect is that the chasing LED appears to "push" the charge
 * 		level upwards.
 *
 * @copyright Copyright MN8 (c) 2023
 * 
 */
//******************************************************************************

#include "ProgressAnimation.h"

#include "esp_log.h"

static const char* TAG = "PA"; // Progress Animation

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 * 
 * @param led_pixels   Pointer to the LED pixels
 * @param start_pixel  Starting pixel
 * @param led_count    Number of LED/pixels to be updated
 *
 * @returns  Number of LEDs updated. Will NOT always be 'led_count'!
 *
 * This class handles the moving/animated motion of the "low" and "high" static
 * segments.
 */
int ProgressAnimation::refresh(uint8_t* led_pixels, int start_pixel, int led_count) {

	if (led_count < 0)
	{
		ESP_LOGE(TAG, "Negative animation refresh! (%d)", led_count);
        led_count = 0;
	}

	if (fill_level == 0)
	{
		animation_leds = led_count;
	}

    low.refresh(led_pixels, start_pixel, fill_level);
    high.refresh(led_pixels, start_pixel + fill_level, (animation_leds - fill_level));

    fill_level++;
    if (fill_level > animation_leds)
    {
    	fill_level = 0;
    }

    return animation_leds;
}

//******************************************************************************
/**
 * @brief Reset the state of the charge progress segment
 * 
 * @param color_fg   Color to set the "charging" LED pixels (foreground)
 * @param color_bg   Color to set the "non-charging" LED pixels (background)
 */
void ProgressAnimation::reset(uint32_t color_fg, uint32_t color_bg) {
    this->color_fg = color_fg;
    this->color_bg = color_bg;
    this->fill_level = 0;
    low.reset(color_fg);
    high.reset(color_bg);
    this->BaseAnimation::reset();
}
