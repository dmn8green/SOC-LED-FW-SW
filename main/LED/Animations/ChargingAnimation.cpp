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
 *      The charging animation split the LEDS in 4 static segments:
 *           - first 2 segments are used for the charging animation
 *           - third segments is 2 blue pixel indicating where the charge is at
 *           - last segment is the amount of charge left
 *
 *      The first segment is solid color at the physical bottom of the LED. Usually at
 *      least one LED, but defined by CHARGE_BASE_LED_CNT. If the charge level is low enough,
 *      only CHARGE_BASE_LED_CNT will be on. As the charge increases, the charge level
 *      indicator may be shown as well, possibly overlapping with the CHARGE_BASE_LED_CNT
 *      LEDs. Until at least one available LED is present between the two indicators, no
        animation is done.
 * 
 *      The following examples are with CHARGE_BASE_LED_CNT=1 and CHARGE_LEVEL_LED_CNT=2
 * 
 *      These charge diagrams can be seen using the main/gtest unit tests with the "-i"
 *      (interactive) mode. See main/gtest/README.
 * 
 *      1) Normal mid-charge (50%): BASE and LEVEL indicators are separated by enough
 *         LEDs to provide a "chase" animation. (B=Blue, >=Animation area):
 *
 *                  0                     1                   2                 3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|>|>|>|>|>|>|>|>|>|>|>|>|>|>|>|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *                  │                               │ │
 *                  │                               │ ├─ Charge Level (top of indicator)
 *                  └─ CHARGE_BASE_LED_CNT          │ │
 *                                                  └─┴─ CHARGE_LEVEL_LED_CNT
 *
 *      2) Below threshold at which any animation can happen:
 *
 *                  1                   2                 3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *        LED BAR: |B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *        LED BAR: |B|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *
 *      3) Minimum animation level:
 *
 *                  0                     1                   2                 3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|>|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 * 
 * @param[in] led_pixels   Pointer to the LED pixels
 * @param[in] led_count    Number of LED pixels to be updated
 * @param[in] start_pixel  Starting pixel
 */
void ChargingAnimation::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {

    int adjusted_charge_pct = MIN ((charge_percent + CHARGE_PERCENT_BUMP), 100);
    int charged_led_count = (((led_count * adjusted_charge_pct) / 100));

    int lowLeds = CHARGE_BASE_LED_CNT;

    int chargeIndicatorLedsShown = charged_led_count - CHARGE_LEVEL_LED_CNT < 0 ? charged_led_count : CHARGE_LEVEL_LED_CNT;

    // (-): overlap  (0): adjacent (+): animation
    int solidSeparation = (charged_led_count - chargeIndicatorLedsShown - CHARGE_BASE_LED_CNT);

    // Animation only if we have positive space between solid indicators
    int animationLeds = 0;
    if (solidSeparation < 0)
    {
        // If indicators overlap, largest one determines base.
        lowLeds = MAX (chargeIndicatorLedsShown, CHARGE_BASE_LED_CNT);
    }
    else if (solidSeparation == 0)
    {
        // Two indicators are adjacent. Still no animation.
        lowLeds = chargeIndicatorLedsShown + CHARGE_BASE_LED_CNT;
    }
    else
    {
        animationLeds = solidSeparation;
    }


    // To keep var name short, we'll use s1, l1, s2, l2, s3, l3, s4, l4 to represent the start and length of each segment
    int s1 = 0;
    int l1 = lowLeds + (charge_anim_pixel_count);

    int s2 = l1;
    int l2 = animationLeds - charge_anim_pixel_count;

    int s3 = animationLeds > 0 ? l1 + l2 : 0;
    int l3 = animationLeds > 0 ? CHARGE_LEVEL_LED_CNT : 0;

    int s4 = l1 + l2 + l3;
    int l4 = (led_count - s4);

    // for test reporting of displayed charge level
    charge_top_leds = s4;

    if (l1 + l2 + l3 + l4 != led_count)
    {
        printf ("\n\nERROR!!!\n\n");
    }

    if (!quiet)
    {
        ESP_LOGD(TAG, "refresh segments: s1:%d-%d s2:%d-%d, s3:%d-%d, s4:%d-%d",
             s1, l1, s2, l2, s3, l3, s4, l4);
    }

    // The refresh takes in the segment's start pixel (offset) and pixel count
    // to refresh
    // TODO: Refactor refresh to take start before count. This will make the code more readable.
    static_charging_solid_animation.refresh (led_pixels, l1, s1);
    static_charging_empty_animation.refresh (led_pixels, l2, s2);
    static_charge_animation.refresh         (led_pixels, l3, s3);
    static_remaining_animation.refresh      (led_pixels, l4, s4);

    // Reset animation to base
    if (++charge_anim_pixel_count > animationLeds) {
        charge_anim_pixel_count = 0;
        if (!quiet)
        {
            ESP_LOGD(TAG, "Resetting charge anim pixel count (for next time)");
        }
    }

    if (simulate_charge) {
        //ESP_LOGI(TAG, "Simulating charge percent %ld %ld", simulated_charge_percent, charge_percent);
        if ((++simulated_charge_percent) % 10 == 0) {
            charge_percent = charge_percent >= 100 ? 0 : charge_percent + 1;
        }
        // TODO: charged_led_count was removed. Is the above sufficient?
#if 0
        if (this->charged_led_count>=led_count) {
            this->charged_led_count = 0;
        }
#endif
    }
}
