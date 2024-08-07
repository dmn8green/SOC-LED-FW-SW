//******************************************************************************
/**
 * @file PulsingAnimation.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief PulsingAnimation class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 */
//******************************************************************************

#include "PulsingAnimation.h"

#include "Utils/HSV2RGB.h"
#include "Utils/Colors.h"

#include "esp_log.h"

static const char* TAG = "PLA";  // Pulse Animation (PA is ProgressAnimation)

//******************************************************************************
/**
 * @brief Reset the state of the animation
 * 
 * @param h                 Hue
 * @param s                 Saturation
 * @param v                 Value
 * @param min_value         Minimum value for the saturation or value
 * @param rate              Number of milliseconds between refreshes
 * @param pulse_saturation  True if the saturation should be pulsed
 * @param pulse_curve       Pointer to the pulse curve
 */
void PulsingAnimation::reset(
	COLOR_HSV *pHsv,
    uint32_t min_value, 
    uint32_t rate, 
    bool pulse_saturation,
    BasePulseCurve* pulse_curve
) {

	this->hsv = *pHsv;

    // this->h = pHsv->h;
    // this->s = pHsv->s;
    // this->v = pHsv->v;

    this->min_value = min_value;
    this->BaseAnimation::set_rate(rate);
    this->pulse_saturation = pulse_saturation;
    this->pulse_curve = pulse_curve;
    ESP_LOGI(TAG, "Pulse curve is %p", this->pulse_curve);

    this->max_value = pulse_saturation ? hsv.s : hsv.v;

    this->pulse_count = 0;
    this->increasing = true;
}

//******************************************************************************
/**
 * @brief Refresh the LED pixels
 * 
 * @param led_pixels   Pointer to the LED pixels
 * @param start_pixel  Starting pixel offset in led_pixels
 * @param led_count    Number of LED pixels to be updated
 */
int PulsingAnimation::refresh(uint8_t* led_pixels, int start_pixel, int led_count) {
    #define AT_MIN_VALUE (this->pulse_count == this->min_value)
    #define AT_MAX_VALUE (this->pulse_count == this->max_value)

    uint32_t red, green, blue;
    uint32_t hue, saturation, value;

    hue = this->hsv.h;
    saturation = this->pulse_saturation ? this->pulse_count : this->hsv.s;
    value = this->pulse_saturation ? this->hsv.v : this->pulse_count;
    // ESP_LOGI(TAG, "HSV: %ld, %ld, %ld", hue, saturation, value);

    // If we are at the min or max value, then we need to change direction.
    uint32_t range = this->max_value - this->min_value;
    uint32_t pulse_rate_idx = 100.0*(this->pulse_count-this->min_value)/range;
    this->set_rate(this->pulse_curve->get_value(pulse_rate_idx) + 10);

    this->pulse_count = this->increasing ? this->pulse_count + 1 : this->pulse_count - 1;
    this->increasing = AT_MIN_VALUE ? true : AT_MAX_VALUE ? false : this->increasing;
    //ESP_LOGI(TAG, "%ld: Pulse count: %ld", this->led_number, this->pulse_count);

    Colors::instance().hsv2rgb(hue, saturation, value, &red, &green, &blue);

    for (int pixel_idx = start_pixel; pixel_idx < start_pixel + led_count; pixel_idx++) {
        // Build RGB pixels
        led_pixels[pixel_idx * 3 + 0] = green;
        led_pixels[pixel_idx * 3 + 1] = red;
        led_pixels[pixel_idx * 3 + 2] = blue;
    }

    return led_count;
}
