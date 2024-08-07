//******************************************************************************
/**
 * @file PulsingAnimation.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief PulsingAnimation class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "BaseAnimation.h"
#include "BasePulseCurve.h"
#include "Utils/Colors.h"

//******************************************************************************
/**
 * @brief Pulsing color animation
 * 
 * This animation pulses the LED pixels between two colors at the specified rate.
 * 
 * The colors are set using the reset() method.
 * 
 * The colors must be specified in hue, saturation, and value.  The hue is a
 * value between 0 and 360.  The saturation and value are between 0 and 100.
 * 
 * The rate is the number of milliseconds between refreshes.  The default rate
 * is 10 milliseconds.
 * 
 * The refresh() method is called by the LED task to update the LED pixels.
 * 
 * The reset method is called by the LED task when the animation is changed.  The
 * pulsing can be performed on the saturation or the value.  The min_value is the
 * minimum value for the saturation or value.  The rate is the number of
 * milliseconds between refreshes.
 */
class PulsingAnimation : public BaseAnimation {
public:
    void reset(
	    COLOR_HSV *pHsv,
        uint32_t min_value = 10, 
        uint32_t rate = 10, 
        bool pulse_saturation = false,
        BasePulseCurve* pulse_curve = nullptr
    );

    int refresh(uint8_t* led_pixels, int start_pixel=0, int led_count=0) override;

private:
    COLOR_HSV hsv;
    // uint32_t h, s, v;

    uint32_t min_value = 0;
    uint32_t max_value = 100;
    bool pulse_saturation = false;

    BasePulseCurve* pulse_curve = nullptr;

    uint32_t pulse_count = 0;
    bool increasing = true;
};
