//******************************************************************************
/**
 * @file ChasingAnimation.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief ChasingAnimation class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "BaseAnimation.h"

//******************************************************************************
/**
 * @brief Chasing color animation
 * 
 * This animation simulates a chasing LED going from start to end at the given
 * rate.
 * 
 * The colors are set using the reset() method.
 *
 */
class ChasingAnimation : public BaseAnimation {
public:
    void reset(uint32_t base_color, uint32_t chasing_color, uint32_t chase_rate = 10);
    void refresh(uint8_t* led_pixels, int start_pixel=0, int led_count=0) override;

private:
    uint32_t base_color;
    uint32_t chasing_color;
    
    uint32_t current_pixel = 0;
};
