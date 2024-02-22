//******************************************************************************
/**
 * @file LedTaskSpi.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief LedTaskSpi class declaration
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "Utils/NoCopy.h"
#include "Utils/Colors.h"
#include "RmtOverSpi.h"

#include "esp_err.h"
#include "driver/spi_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Animations/ChargingAnimation.h"
#include "Animations/StaticAnimation.h"
#include "Animations/SmoothRatePulseCurve.h"
#include "Animations/PulsingAnimation.h"
#include "Animations/ChargingAnimationWhiteBubble.h"

//******************************************************************************
/**
 * @brief LED state
 * 
 * This enum defines the LED state.
 */
typedef enum {
    e_station_available,           // 00 green   (s)  - Available and ready to charge
    e_station_waiting_for_power,   // 01 cyan    (s)  - Waiting for power to be available
    e_station_charging,            // 02 blue    (p)  - Charging a vehicle
    e_station_charging_complete,   // 03 blue    (s)  - Charging complete, or preparing for vehicle communication after plugging a vehicle in 
    e_station_out_of_service,      // 04 red     (s)  - Out of service
    e_station_disable,             // 05 red     (s)  - Disabled
    e_station_booting_up,          // 06 yellow  (p)  - Station booting up / Not ready yet
    e_station_offline,             // 07 white   (s)  - Station offline
    e_station_reserved,            // 08 orange  (s)  - Station reserved
    e_station_iot_unprovisioned,   // 09 purple  (s)  - Station not provisioned with AWS
    e_station_debug_on,            // 10 fushia  (s)  - Debug mode on
    e_station_debug_off,           // 11 black   (s)  - Debug mode off
    e_station_cp_unprovisioned,    // 12 purple  (p)  - ChargePoint not provisioned with AWS
    e_station_waiting_4_first_state, // 13 white  (s)  - Waiting for first state
    e_station_no_connection,       // 14 black   (s)  - No connection
    e_debug_charging,              // 14 blue    (p)  - Debug charging
    e_station_unknown              // Error state
} led_state_t;

//******************************************************************************
/**
 * @brief LED state info
 * 
 * This struct defines the LED state info.
 * 
 * @note This struct is used to pass the LED state info to the LedTaskSpi task.
 * @note The members could have been part of the LedTask class.
 */
typedef struct {
    led_state_t state;
    int charge_percent;
} led_state_info_t;

//******************************************************************************
/**
 * @brief LedTaskSpi class
 * 
 * This class is responsible for controlling the LED strip.
 * 
 * The LED strip is controlled by the SPI peripheral instead of the ESP32 RMT 
 * driver.
 * 
 * The ESP32 RMT driver is known to flicker when wifi is enabled.
 * 
 * The LedTaskSpi is a FreeRTOS task that runs on core 1.
 * 
 * It is responsible to generate the LED pattern based on the state of the 
 * station.
 * 
 * @note We do not allow to change the led count mid way.  When the UI changes
 * the count, it must reboot the device after.
 */
class LedTaskSpi : public NoCopy {
public:
    LedTaskSpi(void) = default;
    ~LedTaskSpi(void) = default;
    
    esp_err_t setup(int led_bar_number, int gpio_pin, spi_host_device_t spi, int led_count, bool disable_connecting_leds);
    esp_err_t start(void);
    esp_err_t resume(void);
    esp_err_t suspend(void);
    esp_err_t set_pattern(led_state_t pattern, int charge_percent);
    esp_err_t set_state(const char* new_state, int charge_percent);

    const char* get_state_as_string(void);

protected:
    void vTaskCodeLed(void);
    static void svTaskCodeLed( void * pvParameters ) { ((LedTaskSpi*)pvParameters)->vTaskCodeLed(); }

private:

    int led_bar_number;
    int gpio_pin;
    int led_count;

    uint8_t* led_pixels;
    led_state_info_t state_info;
    LED_INTENSITY intensity = LED_INTENSITY_HIGH;

    TaskHandle_t task_handle;
    QueueHandle_t state_update_queue;

    BaseAnimation* animation = nullptr;
    ChargingAnimation charging_animation;
    StaticAnimation static_animation;
    SmoothRatePulseCurve smooth_rate_pulse_curve;
    PulsingAnimation pulsing_animation;
    ChargingAnimationWhiteBubble charging_animation_white_bubble;
    bool disable_connecting_leds = false;

    RmtOverSpi rmt_over_spi;
};

