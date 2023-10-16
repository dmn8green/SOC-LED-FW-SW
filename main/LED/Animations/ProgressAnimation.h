//******************************************************************************
/**
 * @file ProgressAnimation.h
 * @author pat laplante (plaplante@appliedlogix.com) 
 *         bill mcguinness (bmcguinness@appliedlogix.com)
 * @brief ProgressAnimationclass definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#pragma once

#include "StaticAnimation.h"

//******************************************************************************
/**
 * @brief Progress Animation 
 * 
 * This animation controls the "chasing" LED as it moved upward from the  
 * base indicator to the charge level. The Progress Animation does not
 * include either of the endpoints.
 * 
 * The color is set using the reset() method.
 */
class ProgressAnimation : public BaseAnimation {
public:
    void reset(uint32_t color_fg, uint32_t color_bg);
    int refresh(uint8_t* led_pixels, int start_pixel=0, int led_count=0) override;

private:
    uint32_t color_fg;
    uint32_t color_bg;

    uint32_t animation_leds;  //
    uint32_t fill_level;      // increases 0 to animation_cnt

    StaticAnimation low;
    StaticAnimation high;
};
