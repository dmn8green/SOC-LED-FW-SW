//******************************************************************************
/**
 * @file ChargingAnimation.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief ChargingAnimation class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "BaseAnimation.h"
#include "StaticAnimation.h"

#include <stdint.h>

// // The solid color indicating charge level. If more than one LED is specified
// // the top/highest one will indicate the charge level
// #define CHARGE_LEVEL_LED_CNT (1)

// // The displayed "Charge Level" is a raw division of charge percentage by the
// // number of LEDs. The mathematically correct division of LEDs is perhaps not
// // the most helpful -- the bar will not show "full" until exactly 100% of charge
// // is achieved, which may never happen. Also, the low-end of the bar has a fixed
// // level (CHARGE_BASE_LED_CNT, above) requiring a higher inital charge to see
// // any perceived progress at charge start. This value defines a "fudge factor"
// // (in the positive direction only) added to a charge level to increase the
// // display level.
// //
// // Leave this value at 0 for "raw" display, or use a value of 1-5
// // to "goose" the charge level a little bit (it's an old circus term).
// //
// #define CHARGE_PERCENT_BUMP  (3)


//******************************************************************************
/**
 * @brief 'Charging' color animation
 * 
 * This animation is used to indicate that the battery is charging.
 */
class ChargingAnimationWhiteBubble : public BaseAnimation {
public:
    void reset(uint32_t static_color_1, uint32_t static_color_2, uint32_t chase_color, uint32_t chase_rate = 10);
    int refresh(uint8_t* led_pixels, int start_pixel = 0, int led_count=0) override;
    void set_charge_percent(uint32_t charge_percent);
    inline void set_charge_simulation (bool set_sim_charge) {this->simulate_charge = set_sim_charge; }
    inline bool get_charge_simulation (void) {return this->simulate_charge; }
#ifdef UNIT_TEST
    inline uint32_t get_charge_percent(void) { return this->charge_percent; }
    inline void set_quiet (bool newVal) { this->quiet = newVal; }
#endif

private:
    StaticAnimation charge_left_of_bubble;
    StaticAnimation charge_bubble;
    StaticAnimation charge_right_of_bubble;
    StaticAnimation charge_remaining;

    uint32_t rate_at_100_percent = 0;

    uint32_t charge_percent = 0;
    uint32_t static_color_1;
    uint32_t static_color_2;
    uint32_t chase_color;
    uint32_t bubble_position = 0; 
    uint32_t bubble_anim_max = 0;	// Animation state (from 0 to charge indicator)
    
    bool simulate_charge = false;
};
