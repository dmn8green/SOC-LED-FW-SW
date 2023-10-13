//******************************************************************************
/**
 * @file ChargeIndicator.h
 * @author pat laplante (plaplante@appliedlogix.com) 
 *         bill mcguinness (bmcguinness@appliedlogix.com)
 *
 * @brief ChargeIndicator class definition
 * 
 *        ChangeIndicator consists of: 
 *        - ProgressAnimation (Dynamic)
 *        - ChargeLevel (Static) 
 *
 *        This class has no knowledge of charge level, but knows
 *        that it should not change its led count unless animation
 *        has completed a cycle.
 * 
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#pragma once

#include "ProgressAnimation.h"
#include "StaticAnimation.h"

//******************************************************************************
/**
 * @brief Charge Indicator 
 * 
 * This animation controls the area of the LED indicating charge. This includes 
 * any animated "chase" patterns AND the static LED charge level indicator.  
 *
 */
class ChargeIndicator: public BaseAnimation {
public:
    void reset(uint32_t color_fg, uint32_t color_bg, uint32_t chase_color, uint32_t ci_led_count);
    int refresh(uint8_t* led_pixels, int start_pixel=0, int led_count=0);

private:
    uint32_t color_fg = 0;
    uint32_t color_bg = 0;
    uint32_t chase_color = 0;

    uint32_t ci_led_count = 0;  // Length of the solid charge indicator at the top
    uint32_t fill_level= 0; 	// Animation state (from 0 to charge indicator)

    ProgressAnimation progress_animation;
    StaticAnimation charge_level;
};
