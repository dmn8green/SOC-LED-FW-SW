//******************************************************************************
/**
 * @file ChargingAnimation.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 *         bill mcguinness (bmcguinness@appliedlogix.com)
 * @brief ChargingAnimation class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 * The top-level class for an LED bar displaying an active charging
 * status.
 *
 *      Object hierarchy. Everything eventually resolved to a static pattern class
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
 *       Diagram of these objects as they appear on a LED strip in mid-charge. Note
 *       the Progress Animation object is "moving" blue pixels up the bar to indicate
 *       charge in progress. Animations like these can be seen using the unit test
 *       in the main/gtest directory (run the program in Interactive mode with the
 *       -i option)
 *
 *            0                   1                   2                   3
 *            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *           |B|B|B|B|B|B|.|.|.|.|.|.|.|.|.|.|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *           |B|B|B|B|B|B|B|.|.|.|.|.|.|.|.|.|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *           |B|B|B|B|B|B|B|B|.|.|.|.|.|.|.|.|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *           |B|B|B|B|B|B|B|B|B|.|.|.|.|.|.|.|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *            │ ├─── L: Low ──┘ └─ H: High ───┤ │ │                         │
 *            │ ├─── PA: Progress Animation ──┤ │ └──────── T: Top ─────────┤
 *            │ └──  CI: Charge Indicator  ───┴┬┘                           │
 *            ├─ B (Charge 'base')             └── CL: Charge Level         │
 *            └─────────────────── CA: (Charge Animation) ──────────────────┘
 *
 */

#include <algorithm>
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
 *
 *      2) Below threshold at which any animation will happen:
 *
 *                  0                   1                   2                   3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *        LED BAR: |B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *        LED BAR: |B|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *
 *      3) Minimum animation level. Note that this appears as a single "blinking" LED
 *         between the Base and Charge Level Indicator.
 *
 *                  0                   1                   2                   3
 *                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *        LED BAR: |B|>|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 */
int ChargingAnimation::refresh(uint8_t* led_pixels, int start_pixel, int led_count) {

    int adjusted_charge_pct = 0;

    // TODO: Should probably use a sliding value of charge bump based on LED bar
    //       length; for now using our fixed value from 31/32 LED live tests if
    //       we have a similar length.
    if (led_count < 40)
    {
        adjusted_charge_pct = MIN ((charge_percent + CHARGE_PERCENT_BUMP), 100);
    }
    else
    {
        adjusted_charge_pct = charge_percent;
    }

    // At full animation (or without animation), number of LEDs lit for this charge level
    // this is the top of our second segment
    int charged_led_count = (((led_count * adjusted_charge_pct) / 100));

    // Fill in all needed segments. First one's easy
    int filled_leds = base.refresh (led_pixels, 0, CHARGE_BASE_LED_CNT);

    // The charge indicator, including "short" versions at low charge levels (no animation)
    filled_leds += charge_indicator.refresh (led_pixels, filled_leds,
            (std::max (0, (charged_led_count - CHARGE_BASE_LED_CNT))));

    // Everything left over
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
