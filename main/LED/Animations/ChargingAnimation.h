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
#include "PulsingAnimation.h"
#include "ChasingAnimation.h"

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
    void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) override;
    inline void set_charge_percent(uint32_t charge_percent) { this->charge_percent = charge_percent; }

private:
    StaticAnimation static_charge_animation;
    StaticAnimation static_remaining_animation;
    ChasingAnimation chasing_animation;
    uint32_t charge_percent;
    uint32_t static_color_1;
    uint32_t static_color_2;
    uint32_t chase_color;

    bool simulate_charge = true;
    uint32_t simulated_charge_percent = 0;
};
