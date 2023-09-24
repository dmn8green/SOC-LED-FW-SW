//******************************************************************************
/**
 * @file BaseAnimation.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief BaseAnimation class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#pragma once

#include "Utils/NoCopy.h"

#include <stdint.h>

#include "freertos/FreeRTOS.h"

//******************************************************************************
/**
 * @brief Base class for all animations
 * 
 * This class is used to define the interface for all animations.  It is used
 * by the LED task to refresh the LED pixels.
 * 
 * This class is designed to be re-used to avoid memory fragmentation.  The
 * reset method is called when the animation is changed.  This allows the
 * animation to reset any internal state.
 * 
 * The refresh() method is called by the LED task to update the LED pixels.
 * 
 * The rate is used to control the speed of the animation.  The rate is the
 * number of milliseconds between refreshes.  The default rate is portMAX_DELAY
 * which means that the animation will only be refreshed when the rate is
 * changed.
 * 
 * The reset() method is called by the LED task when the animation is changed.
 * This allows the animation to reset any internal state.
 * 
 * The set_rate() and get_rate() methods are used to set and get the rate.
 * 
 * None of the animations actually update the LED hardware.  They only update
 * the LED pixels.  The LED task is responsible for updating the LED hardware.
 */
class BaseAnimation : public NoCopy {
public:
    virtual void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) = 0;

    inline uint32_t get_rate(void) { return this->rate; }
    inline void set_rate(uint32_t rate) { this->rate = rate; }

    virtual void reset(void) { this->rate = portMAX_DELAY; }

private:
    uint32_t rate = portMAX_DELAY;
};
