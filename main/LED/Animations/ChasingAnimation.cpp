//******************************************************************************
/**
 * @file ChasingAnimation.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief ChasingAnimation class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 */

#include "ChasingAnimation.h"

#include "esp_log.h"

static const char* TAG = "ChasingAnimation";

//******************************************************************************
/**
 * @brief Reset the state of the animation
 * 
 * @param base_color 
 * @param chasing_color 
 * @param rate 
 */
void ChasingAnimation::reset(
    uint32_t base_color,
    uint32_t chasing_color,
    uint32_t chase_rate
) {
    this->base_color = base_color;
    this->chasing_color = chasing_color;
    this->current_pixel = 0;
    this->BaseAnimation::set_rate(chase_rate);
}

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 * 
 * @param led_pixels   Pointer to the LED pixels
 * @param led_count    Number of LED pixels to be updated
 * @param start_pixel  Starting pixel
 */
void ChasingAnimation::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {
    for (int i = 0; i < led_count; i++) {
        led_pixels[i] = this->base_color;
    }

    // ESP_LOGI(TAG, "current_pixel: %ld, start_pixel %d, led_count %d", this->current_pixel, start_pixel, led_count);
    led_pixels[(this->current_pixel + start_pixel)] = this->chasing_color;

    if (++this->current_pixel >= led_count) {
        this->current_pixel = 0;
    }
}