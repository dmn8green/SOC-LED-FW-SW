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

#include "ChargingAnimation.h"

#include "esp_log.h"

static const char* TAG = "ChargingAnimation";

//******************************************************************************
/**
 * @brief Reset the state of the animation
 * 
 * @param color_start 
 * @param color_end 
 * @param rate 
 */
void ChargingAnimation::reset(
    uint32_t static_color_1,
    uint32_t static_color_2,
    uint32_t chase_color,
    uint32_t chase_rate
) {
    this->static_color_1 = static_color_1;
    this->static_color_2 = static_color_2;
    this->chase_color = chase_color;

    this->charge_percent = charge_percent;
    this->BaseAnimation::set_rate(chase_rate);

    this->static_charge_animation.reset(static_color_1);
    this->static_remaining_animation.reset(static_color_2);
    this->chasing_animation.reset(chase_color, 0XFFFFFFFF);
    this->chasing_animation.reset(0XFFFFFFFF, chase_color);
}

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 * 
 * @param led_pixels   Pointer to the LED pixels
 * @param led_count    Number of LED pixels to be updated
 * @param start_pixel  Starting pixel
 */
void ChargingAnimation::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {
    int charged_led_count = (led_count * this->charge_percent) / 100;
    int charging_pix_count = 1;
    int uncharged_led_count = led_count - (charged_led_count + charging_pix_count); // substract the charging pixel

    this->chasing_animation.refresh         (led_pixels, charged_led_count, 0);
    this->static_charge_animation.refresh   (led_pixels, charged_led_count+charging_pix_count, charged_led_count);
    this->static_remaining_animation.refresh(led_pixels, led_count, charged_led_count + charging_pix_count);

    if (simulate_charge) {
        //ESP_LOGI(TAG, "Simulating charge percent %ld %ld", simulated_charge_percent, charge_percent);
        if ((++simulated_charge_percent) % 10 == 0) {
            charge_percent = charge_percent == 100 ? 0 : charge_percent + 1;    
        }
    }
}