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
    for (int i = start_pixel; i < led_count; i++) {
        led_pixels[i * 3 + 0] = (this->base_color >> 16) & 0xFF;
        led_pixels[i * 3 + 1] = (this->base_color >> 8) & 0xFF;
        led_pixels[i * 3 + 2] = this->base_color & 0xFF;
    }
    
    led_pixels[((this->current_pixel + start_pixel)*3) + 0] = (this->chasing_color >> 16) & 0xFF;
    led_pixels[((this->current_pixel + start_pixel)*3) + 1] = (this->chasing_color >> 8) & 0xFF;
    led_pixels[((this->current_pixel + start_pixel)*3) + 2] = this->chasing_color & 0xFF;

    if (++this->current_pixel >= led_count) {
        this->current_pixel = 0;
    }
}