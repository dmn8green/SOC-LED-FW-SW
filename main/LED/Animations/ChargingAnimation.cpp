//******************************************************************************
/**
 * @file ChargingAnimation.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief ChargingAnimation class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 */

#include "ChargingAnimation.h"

#include <inttypes.h>

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
 * The charging animation split the LEDS in 4 static segments:
 *   The first 2 segments are used for the charging animation
 *   The third segments is 2 blue pixel indicating where the charge is at
 *   The last segment is the amount of charge left
 * 
 * The first pixel is always blue, followed by the animation
 *
 * When charge is between 0 and 12% (3 pixels), there are no animation since
 * only 3 pixels are visible.
 *   - first pixel always blue
 *   - 2 pixel to represent the charge.
 * 
 * @note There is a glitch visible when wrapping around from 100 to 0.  This
 *       will never be visible in the field since the charge will never wrap
 *       around.
 * 
 * @param led_pixels   Pointer to the LED pixels
 * @param led_count    Number of LED pixels to be updated
 * @param start_pixel  Starting pixel
 */
void ChargingAnimation::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {

    // +1 to make sure we always have at least 1 pixel on
    int new_charged_led_count = (((led_count * this->charge_percent) / 100) + 1);
    if (new_charged_led_count>led_count) {
        new_charged_led_count = led_count;
    }

    if (!quiet)
    {
        ESP_LOGD(TAG, "refresh start: %d %" PRIu32 " %" PRIu32,
            new_charged_led_count,
            this->charged_led_count,
            this->charge_anim_pixel_count);
    }

    // if (new_charged_led_count != this->charged_led_count && this->charged_led_count == this->charge_anim_pixel_count) {
    if (new_charged_led_count != this->charged_led_count) {

        if (!quiet)
        {
            ESP_LOGD(TAG, "INCREASING CHARGED LED COUNT from %" PRIu32 " to %d", this->charged_led_count, new_charged_led_count);
        }
        this->charged_led_count = new_charged_led_count;
    }

    // To keep var name short, we'll use anim_px to represent the number of pixels
    // and s1, l1, s2, l2, s3, l3, s4, l4 to represent the start and length of each segment
    int anim_px = (this->charged_led_count - this->charge_anim_pixel_count) - 1;
    int s1 = 0;
    int l1 = this->charged_led_count-anim_px;
    int s2 = this->charge_anim_pixel_count + 1;
    int l2 = anim_px;
    int s3 = this->charged_led_count;
    int l3 = 2;
    int s4 = s3 + 2;
    int l4 = s4 >= led_count ? 0 : led_count - this->charged_led_count - 2;

    if (!quiet)
    {
        ESP_LOGD(TAG, "refresh segments: %" PRIu32 " %" PRIu32 " %" PRIu32 "  %d: %d-%d, %d-%d, %d-%d, %d-%d",
            this->charge_percent,
            this->charged_led_count,
            this->charge_anim_pixel_count,
            anim_px,
            s1, l1, s2, l2, s3, l3, s4, l4);
    }

    // The refresh takes in the start pixel and last pixel to refresh
    // TODO Refactor refresh to take start before last. This will make the code more readable.
    this->static_charging_solid_animation.refresh(led_pixels, 0+l1, s1);
    this->static_charging_empty_animation.refresh(led_pixels, l1+l2, s2);
    this->static_charge_animation.refresh        (led_pixels, l1+l2+l3, s3);
    this->static_remaining_animation.refresh     (led_pixels, l1+l2+l3+l4, s4);

    if (++this->charge_anim_pixel_count > this->charged_led_count) {
        this->charge_anim_pixel_count = 0;
        if (!quiet)
        {
            ESP_LOGD(TAG, "Resetting charge anim pixel count");
        }
    }

    if (simulate_charge) {
        //ESP_LOGI(TAG, "Simulating charge percent %ld %ld", simulated_charge_percent, charge_percent);
        if ((++simulated_charge_percent) % 10 == 0) {
            charge_percent = charge_percent >= 100 ? 0 : charge_percent + 1;
        }
        if (this->charged_led_count>=led_count) {
            this->charged_led_count = 0;
        }
    }
}
