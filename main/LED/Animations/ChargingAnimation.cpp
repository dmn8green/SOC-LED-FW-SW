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
 * The top-level class for an LED bar displaying an active charging
 * status.
 *
 * TODO:  Object hierarchy. Everything eventually is a static
 *        animation class.
 *
 *        CA: Charge Animation (this class)
 *            B: Base (static)
 *            CI: Charge Indicator
 *                PA: Progress Animation
 *                   L: Low (static)
 *                   H: High (static)
 *                CL: Charge Level Indicator (static)
 *            T: Top (static)
 *
 */

#include "ChargingAnimation.h"

#include <stdio.h>
#include <inttypes.h>

#include "esp_log.h"

static const char* TAG = "CA"; // ChargingAnimation

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

    this->BaseAnimation::set_rate(chase_rate);

    this->base.reset(static_color_1);
    this->charge_indicator.reset(static_color_1, static_color_2, chase_color, CHARGE_LEVEL_LED_CNT);
    this->top.reset(static_color_2);
}

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 *
 * @param[in] led_pixels   Pointer to the LED pixels
 * @param[in] led_count    Number of LED pixels to be updated
 * @param[in] start_pixel  Starting pixel
 *
 *      The charging animation split the LEDS into three segments:
 *           - A "base" from which any animation will appear to originate
 *           - A "charge indicator" that includes animation AND the charge level indicator
 *           - A "top" segment with the background color
 *
 *      The "base" segment is always least one LED, and defined by CHARGE_BASE_LED_CNT.
 *      If the charge level is low enough, only CHARGE_BASE_LED_CNT will be on. As the
 *      charge increases, the charge level is incorporated into the base. There can be
 *      overlap of the CHARGE_BASE_LED_CNT and CHARGE_LEVEL_LED_CNT. Until at least one
 *      available LED is present between the two indicators, no animation is done.
 * 
 *      The following examples are with CHARGE_BASE_LED_CNT=1 and CHARGE_LEVEL_LED_CNT=2
 * 
 *      These charge diagrams can be seen using the main/gtest unit tests with the "-i"
 *      (interactive) mode. See main/gtest/README.
 * 
 *      1) Normal mid-charge (50%): BASE and LEVEL indicators are separated by enough
 *         LEDs to provide a "chase" animation. (B=Blue, >=Animation area):
 *
 *                  0                   1                   2                   3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|>|>|>|>|>|>|>|>|>|>|>|>|>|>|>|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *                  │                               │ │
 *                  │                               │ ├─ Charge Level (top of indicator)
 *                  └─ CHARGE_BASE_LED_CNT          │ │
 *                                                  └─┴─ CHARGE_LEVEL_LED_CNT
 *
 *      2) Below threshold at which any animation will happen:
 *
 *                  0                   1                   2                   3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *        LED BAR: |B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *        LED BAR: |B|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *
 *      3) Minimum animation level:
 *
 *                  0                   1                   2                   3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|>|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 */
int ChargingAnimation::refresh(uint8_t* led_pixels, int start_pixel, int led_count) {

    int adjusted_charge_pct = MIN ((charge_percent + CHARGE_PERCENT_BUMP), 100);
    int charged_led_count = (((led_count * adjusted_charge_pct) / 100));

    // If there is no animation, we will handle the "base" level adjustment here (this
    // happens at low charge levels only
    int low_leds = CHARGE_BASE_LED_CNT;
    int ci_leds_shown = charged_led_count - CHARGE_LEVEL_LED_CNT < 0 ? charged_led_count : CHARGE_LEVEL_LED_CNT;

    // (-): overlap  (0): adjacent (+): animation
    int solid_separation = (charged_led_count - ci_leds_shown - CHARGE_BASE_LED_CNT);

    // Animation only if we have positive space between solid indicators
    int animation_leds = 0;
    if (solid_separation < 0)
    {
        // If indicators overlap, largest one determines base.
        low_leds = MAX (ci_leds_shown, CHARGE_BASE_LED_CNT);
    }
    else if (solid_separation == 0)
    {
        // Indicators are adjacent. Still no animation.
        low_leds = ci_leds_shown + CHARGE_BASE_LED_CNT;
    }
    else
    {
        // We have animation!
        animation_leds = solid_separation;
    }

    // Fill in all needed segments.
    int filled_leds = 0;
    filled_leds += base.refresh (led_pixels, 0, low_leds);

    filled_leds += charge_indicator.refresh (led_pixels, low_leds, animation_leds > 0 ? animation_leds + CHARGE_LEVEL_LED_CNT : 0);

    top.refresh (led_pixels, filled_leds, (led_count - filled_leds));

    if (simulate_charge) {

        // Every 10 cycles, bump the charge level or revert to zero

        if ((++simulated_charge_counter) % 10 == 0) {
            charge_percent++;
            if (charge_percent > 100)
            {
                charge_percent = 0;
            }
            ESP_LOGI(TAG, "Simulating charge percent %" PRIu32, charge_percent);
        }
    }

    return led_count;
}
