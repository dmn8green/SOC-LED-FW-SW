//******************************************************************************
/**
 * @file StaticAnimation.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief StaticAnimation class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 */
//******************************************************************************

#include "StaticAnimation.h"

#include "esp_log.h"

static const char* TAG = "SA";  // Static Animation

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 * 
 * @param led_pixels   Pointer to the LED pixels
 * @param start_pixel  Starting pixel
 * @param led_count    Number of LED pixels to be updated
 *
 * @return Number of LEDS updated
 */
int StaticAnimation::refresh(uint8_t* led_pixels, int start_pixel, int led_count) {

    if (led_pixels == nullptr) 
    {
        ESP_LOGE (TAG, "refresh null pointer");
        return 0; 
    }

    if (led_count < 0)
    {
        ESP_LOGE (TAG, "negative request");
        return 0;
    }

    for (int i = start_pixel; i < (start_pixel + led_count); i++) {
        // ESP_LOGI(TAG, "i: %d r: %ld g: %ld b: %ld\n", i, (this->color >> 16) & 0xFF, (this->color >> 8) & 0xFF, this->color & 0xFF);
        led_pixels[i * 3 + 0] = (this->color >> 8) & 0xFF;
        led_pixels[i * 3 + 1] = (this->color >> 16) & 0xFF;
        led_pixels[i * 3 + 2] = this->color & 0xFF;
    }

    return led_count;
}

//******************************************************************************
/**
 * @brief Reset the state of the animation
 * 
 * @param color Color to set the LED pixels
 */
void StaticAnimation::reset(uint32_t color) {
    this->color = color;
    this->BaseAnimation::reset();
}
