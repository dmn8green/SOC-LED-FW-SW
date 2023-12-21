//******************************************************************************
/**
 * @file ChargingAnimationWhiteBubble.cpp
 * @brief ChargingAnimationWhiteBubble class implementation
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 *      The top-level class for an LED bar displaying an active charging status.
 *      This is the second charging animation, which is a "white bubble" animation.
 *      The first charging animation is a "blue bubble" animation.
 *
 *      This animation shows a "bubble" of white pixels moving up the LED bar
 *      to indicate charging in progress. The bubble is a fixed size, and
 *      moves up the bar at a fixed rate, from the bottom to the charge level.
 *
 *      Object hierarchy. Everything eventually resolves to a static pattern class
 *
 *        CAWB: Charge Animation (this class)
 *            B: Base (static)
 *            CI: Charge Indicator
 *                PA: Progress Animation
 *                   L: Low (static)
 *                   H: High (static)
 *                CL: Charge Level Indicator (static)
 *            T: Top (static)
 *
 *       Diagram of these objects as they appear on a 32-controller LED strip in
 *       mid-charge. Note the Progress Animation object is "moving" blue pixels
 *       up the bar to indicate charge in progress.
 *
 *            0                   1                   2                   3
 *            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *           |B|B|B|B|B|B|.|B|B|B|B|B|B|B|B|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *           |B|B|B|B|B|B|B|.|B|B|B|B|B|B|B|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *           |B|B|B|B|B|B|B|B|.|B|B|B|B|B|B|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *           |B|B|B|B|B|B|B|B|B|.|B|B|B|B|B|B|B|.|.|.|.|.|.|.|.|.|.|.|.|.|.|.|
 *            │ ├─── L: Low ──┘  └ H: High ─┤ │   │                         │
 *            │ ├── PA: Progress Animation ─┘ │   └──────── T: Top ─────────┤
 *            │ └──  CI: Charge Indicator  ───┤                             │
 *            ├─ B (Charge 'base')            └─ CL: Charge Level           │
 *            └─────────────────── CA: (Charge Animation) ──────────────────┘
 *
 *        Animations like the above can be seen using the 'led_test' unit test
 *        in "interactive" mode (-1 option).
 *
 */
//******************************************************************************

#include <algorithm>
#include "ChargingAnimationWhiteBubble.h"

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
void ChargingAnimationWhiteBubble::reset(
    uint32_t static_color_1,
    uint32_t static_color_2,
    uint32_t chase_color,
    uint32_t chase_rate
) {
    this->static_color_1 = static_color_1;
    this->static_color_2 = static_color_2;
    this->chase_color = chase_color;
    this->rate_at_100_percent = chase_rate;
    // this->BaseAnimation::set_rate(chase_rate);

    this->charge_left_of_bubble.reset(static_color_1);
    this->charge_bubble.reset(chase_color);
    this->charge_right_of_bubble.reset(static_color_1);
    this->charge_remaining.reset(static_color_2);

    bubble_position = 0;
    bubble_anim_max = 0;
}

//******************************************************************************
void ChargingAnimationWhiteBubble::set_charge_percent(uint32_t charge_percent) {
    if (this->charge_percent < charge_percent) {
        // ESP_LOGI(TAG, "ChargingAnimationWhiteBubble::set_charge_percent: charge_percent %" PRIu32, charge_percent);
        // Force a reset of the animation
        bubble_anim_max = 0;
        bubble_position = 0;
    }

    this->charge_percent = charge_percent;
    if (charge_percent > 100) {
        charge_percent = 100;
    } else if (charge_percent <= 5) {
        charge_percent = 5;
    }

    // Adjust rate logarithmically to make the animation look better
    // at lower charge levels
    // rate_at_100_percent = chase_rate
    // rate_at_50_percent = chase_rate * 2
    // rate_at_25_percent = chase_rate * 4
    // rate_at_10_percent = chase_rate * 10
    // rate_at_5_percent = chase_rate * 20
    // rate_at_1_percent = chase_rate * 100
    set_rate(rate_at_100_percent * (100 / charge_percent));
}

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 *
 * @param[in] led_pixels   Pointer to the LED pixels
 * @param[in] led_count    Number of LED pixels to be updated
 * @param[in] start_pixel  Starting pixel
 */
int ChargingAnimationWhiteBubble::refresh(uint8_t* led_pixels, int start_pixel, int led_count) {

    // Move the bubble up the bar
    if (bubble_position++ >= bubble_anim_max) {
        // ESP_LOGI(TAG, "ChargingAnimationWhiteBubble::refresh: charge_percent %" PRIu32, charge_percent);
        bubble_anim_max = charge_percent / 100.0 * led_count;
        if (bubble_anim_max == 0) {
            bubble_anim_max = 1;
        }
        bubble_position = 0;
    }

    charge_left_of_bubble.refresh(led_pixels, 0, bubble_position);
    charge_bubble.refresh(led_pixels, bubble_position, 1);
    charge_right_of_bubble.refresh(led_pixels, bubble_position + 1, (bubble_anim_max - bubble_position));
    charge_remaining.refresh(led_pixels, bubble_anim_max, (led_count - bubble_anim_max));

    // if (simulate_charge) {
    //     // Every 10 cycles, bump the charge level or revert to zero
    //     if ((++simulated_charge_counter) % 10 == 0) {
    //         charge_percent++;
    //         if (charge_percent > 100)
    //         {
    //             charge_percent = 0;
    //         }
    //         ESP_LOGI(TAG, "Simulating charge percent %" PRIu32, charge_percent);
    //     }
    // }

    return led_count;
}
