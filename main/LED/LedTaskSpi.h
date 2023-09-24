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
#include "RmtOverSpi.h"

#include "esp_err.h"
#include "driver/spi_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Animations/ChasingAnimation.h"
#include "Animations/ChargingAnimation.h"
#include "Animations/StaticAnimation.h"
#include "Animations/SmoothRatePulseCurve.h"
#include "Animations/PulsingAnimation.h"

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
    e_station_status_unknown       // Error state
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
 */
class LedTaskSpi : public NoCopy {
public:
    LedTaskSpi(void) = default;
    ~LedTaskSpi(void) = default;
    
    esp_err_t setup(int led_number, int gpio_pin, spi_host_device_t spi);
    esp_err_t start(void);
    esp_err_t resume(void);
    esp_err_t suspend(void);
    esp_err_t set_pattern(led_state_t pattern, int charge_percent);
    esp_err_t set_state(const char* new_state, int charge_percent);

protected:
    void vTaskCodeLed(void);
    static void svTaskCodeLed( void * pvParameters ) { ((LedTaskSpi*)pvParameters)->vTaskCodeLed(); }

private:

    int led_number;
    int gpio_pin;
    uint8_t* led_pixels;
    led_state_info_t state_info;

    TaskHandle_t task_handle;
    QueueHandle_t state_update_queue;

    BaseAnimation* animation = nullptr;
    ChasingAnimation chasing_animation;
    ChargingAnimation charging_animation;
    StaticAnimation static_animation;
    SmoothRatePulseCurve smooth_rate_pulse_curve;
    PulsingAnimation pulsing_animation;

    RmtOverSpi rmt_over_spi;
};

