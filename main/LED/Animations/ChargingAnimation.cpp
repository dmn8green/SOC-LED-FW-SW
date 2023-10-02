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

    this->static_charging_solid_animation.reset(chase_color);
    this->static_charging_empty_animation.reset(static_color_2);
    this->static_charge_animation.reset(static_color_1);
    this->static_remaining_animation.reset(static_color_2);
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
    bool increased = false;
    int new_charged_led_count = (led_count * this->charge_percent) / 100;
    if (new_charged_led_count > this->charged_led_count && this->charged_led_count == this->charge_anim_pixel_count) {
        this->charged_led_count = new_charged_led_count;
        increased = true;
    }
    
    // anim_px = (charge-p)-1
    // const s1 = 0;
    // const l1 = (charge-anim_px);
    // const s2 = p+1;
    // const l2 = anim_px;
    // const s3 = charge;
    // const l3 = 2;
    // const s4 = s3+2;
    // const l4 = s4 >= 33 ? 0 : 33-charge-2;

    int animated_pixel  = (this->charged_led_count-this->charge_anim_pixel_count) - 1;
    int s1 = 0;
    int l1 = this->charged_led_count - animated_pixel;
    int s2 = this->charge_anim_pixel_count + 1;
    int l2 = animated_pixel;
    int s3 = this->charged_led_count;
    int l3 = 2;
    int s4 = s3 + 2;
    int l4 = s4 >= led_count ? 0 : led_count - s3 - 2;

    this->static_charging_solid_animation.refresh(led_pixels, l1, s1);
    this->static_charging_empty_animation.refresh(led_pixels, l2, s2);
    this->static_charge_animation.refresh        (led_pixels, l3, s3);
    this->static_remaining_animation.refresh     (led_pixels, l4, s4);

    if (++charge_anim_pixel_count > charged_led_count) {
        charge_anim_pixel_count = 0;
    }

    if (simulate_charge) {
        //ESP_LOGI(TAG, "Simulating charge percent %ld %ld", simulated_charge_percent, charge_percent);
        if ((++simulated_charge_percent) % 10 == 0) {
            charge_percent = charge_percent == 100 ? 0 : charge_percent + 1;    
        }
    }
}