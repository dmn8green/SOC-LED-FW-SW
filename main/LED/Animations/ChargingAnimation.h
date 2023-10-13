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
#include "ChargeIndicator.h"

#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)

// The solid color at the base of the LED bar, regardless of charge.
#define CHARGE_BASE_LED_CNT   (1)

// The solid color indicating charge level. If more than one LED is specified
// the top/highest one will indicate the charge level
#define CHARGE_LEVEL_LED_CNT (2)

// The displayed "Charge Level" is a raw division of charge percentage by the
// number of LEDs. The mathematically correct division of LEDs is perhaps not
// the most helpful -- the bar will not show "full" until exactly 100% of charge
// is achieved, which may never happen. Also, the low-end of the bar has a fixed
// level (CHARGE_BASE_LED_CNT, above) requiring a higher inital charge to see
// any perceived progress at charge start. This value defines a "fudge factor"
// (in the positive direction only) added to a charge level to increase the
// display level.
//
// Leave this value at 0 for "raw" display, or use a value of 1-5
// to "goose" the charge level a little bit (it's an old circus term).
//
#define CHARGE_PERCENT_BUMP  (3)


//******************************************************************************
/**
 * @brief 'Charging' color animation
 * 
 * This animation is used to indicate that the battery is charging.
 * 
 * The animation is split in 3 different animations.  A dynamic part (pulsing)
 * and two static parts (charging pixel, and remaining).
 * 
 * The dynamic part is a pulsing animation that is used to indicate that the
 * battery is charging.  The pulsing is performed on the saturation of the
 * color.  The pulsing is performed using the start color. The end color is
 * used for the first static part, the end color is used for the second static
 * part.
 * 
 * This class aggregates other animations to perform the charging animation.
 * 
 * The charging animation is performed as follows where s is the start color,
 * e is the end color, and ssssss>>>> is the animation part:
 * |ssssssssssssss>>>>|s|eeeeeeeeeeeeeee|
 */
class ChargingAnimation : public BaseAnimation {
public:
    void reset(uint32_t static_color_1, uint32_t static_color_2, uint32_t chase_color, uint32_t chase_rate = 10);
    int refresh(uint8_t* led_pixels, int start_pixel = 0, int led_count=0) override;
    inline void set_charge_percent(uint32_t charge_percent) { this->charge_percent = charge_percent <= 100 ? charge_percent:100; }
    inline void set_charge_simulation (bool set_sim_charge) {this->simulate_charge = set_sim_charge; }
    inline bool get_charge_simulation (void) {return this->simulate_charge; }
#ifdef UNIT_TEST
    inline uint32_t get_charge_percent(void) { return this->charge_percent; }
    inline void set_quiet (bool newVal) { this->quiet = newVal; }
#endif

private:
    StaticAnimation base;
    ChargeIndicator charge_indicator;
    StaticAnimation top;

    uint32_t charge_percent = 0;
    uint32_t static_color_1;
    uint32_t static_color_2;
    uint32_t chase_color;

    bool simulate_charge = false;
    bool quiet = false;
    uint32_t simulated_charge_counter = 0;

};
