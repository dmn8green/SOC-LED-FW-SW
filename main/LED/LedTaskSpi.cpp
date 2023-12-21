//******************************************************************************
/**
 * @file LedTaskSpi.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief LedTaskSpi class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#include "Led.h"
#include "LedTaskSpi.h"

#include "Utils/Colors.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_rom_gpio.h"
#include "soc/spi_periph.h"
#include "hal/gpio_types.h"

#include <memory.h>
#include <math.h>

static const char *TAG = "LedTaskSpi";

// Gotta love meta programming.
#define STATIC_ANIM_CASE(x, colorRgb) \
    case x: \
        ESP_LOGI(TAG, "%d: Switching to static anim state " #x " with the color " #colorRgb, this->led_bar_number); \
        this->static_animation.reset(colors.getRgb(colorRgb)); \
        this->animation = &this->static_animation; \
        break;


//******************************************************************************
/**
 * @brief LedTask FreeRTOS task code
 */
void LedTaskSpi::vTaskCodeLed()
{
    TickType_t queue_timeout = portMAX_DELAY;

    // Start with a state changed. On boot, the state is set to e_station_booting_up
    // in the setup phase. This will cause the LED to be set to yellow.
    bool state_changed = true;

    led_state_info_t updated_state;
    LED_INTENSITY current_intensity = Colors::instance().getMode();
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Starting LED task %d", this->led_bar_number);
    this->intensity = current_intensity;

    do
    {
        if (state_changed)
        {
            Colors& colors = Colors::instance();
            ESP_LOGI(TAG, "%d: State changed", this->led_bar_number);
            ESP_LOGI(TAG, "New state is %d", this->state_info.state);
            switch (state_info.state)
            {
            STATIC_ANIM_CASE(e_station_available,           LED_COLOR_GREEN);
            STATIC_ANIM_CASE(e_station_waiting_for_power,   LED_COLOR_CYAN);
            STATIC_ANIM_CASE(e_station_charging_complete,   LED_COLOR_BLUE);
            STATIC_ANIM_CASE(e_station_out_of_service,      LED_COLOR_RED);
            STATIC_ANIM_CASE(e_station_disable,             LED_COLOR_RED);
            STATIC_ANIM_CASE(e_station_offline,             LED_COLOR_WHITE);
            STATIC_ANIM_CASE(e_station_reserved,            LED_COLOR_ORANGE);
            STATIC_ANIM_CASE(e_station_debug_on,            LED_COLOR_DEBUG_ON);
            STATIC_ANIM_CASE(e_station_debug_off,           LED_COLOR_BLACK);
            STATIC_ANIM_CASE(e_station_unknown,             LED_COLOR_PURPLE);
            STATIC_ANIM_CASE(e_station_iot_unprovisioned,   LED_COLOR_PURPLE);
            case e_station_charging:
                ESP_LOGI(TAG, "%d: Charging", this->led_bar_number);
                if (this->animation != &this->charging_animation_white_bubble) {
                    this->charging_animation_white_bubble.reset(colors.getRgb(LED_COLOR_BLUE),
                        colors.getRgb(LED_COLOR_WHITE),
                        colors.getRgb(LED_COLOR_WHITE), 50);
                    this->animation = &this->charging_animation_white_bubble;
                }
                this->charging_animation_white_bubble.set_charge_percent(state_info.charge_percent);
                break;
            case e_station_booting_up:
                {
                    //ESP_LOGI(TAG, "%d: Station is booting up", this->led_bar_number);
                    COLOR_HSV *pHsv = colors.getHsv(LED_COLOR_YELLOW);
                    this->pulsing_animation.reset(pHsv, 0, 20, false, &smooth_rate_pulse_curve);
                    this->animation = &this->pulsing_animation;
                }
                break;

            default:
                ESP_LOGE(TAG, "%d: Unknown state %d", this->led_bar_number, state_info.state);
                break;
            }

            state_changed = false;

            // TODO: FIGURE OUT WHAT TO DO IF WE CAN'T CHANGE THE LED STATE
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "%d: Failed to show state %d", this->led_bar_number, state_info.state);
            }
        }

        if (this->animation != NULL) {
            this->animation->refresh(this->led_pixels, 0, this->led_count);
            this->rmt_over_spi.write_led_value_to_strip(this->led_pixels);
            queue_timeout = this->animation->get_rate() / portTICK_PERIOD_MS;
            ESP_LOGD(TAG, "%d: Queue timeout: %ld", this->led_bar_number, queue_timeout);
        }

        // We use the queue to get the new state of the station.  We also use the queue to
        // to control the refresh rate of the LED strip.
        if (xQueueReceive(this->state_update_queue, &updated_state, queue_timeout) == pdTRUE) {
            ESP_LOGD(TAG, "%d: Switching to state %d", this->led_bar_number, updated_state.state);
            if (state_info.state != updated_state.state || 
                state_info.charge_percent != updated_state.charge_percent
            ) {
                state_info = updated_state;
                state_changed = true;
            } else {
                state_changed = false;
            }
        } else {
            ESP_LOGD(TAG, "%d: Timeout", this->led_bar_number);

            // I don't like that this is a singleton.  Should have pushed the message through a queue.
            // Design of light sensor was an after thought.
            current_intensity = Colors::instance().getMode();
            ESP_LOGD(TAG, "%d: Current intensity %d prev intensity %d", this->led_bar_number, current_intensity, this->intensity);

            if (current_intensity != this->intensity) {
                this->intensity = current_intensity;
                state_changed = true;
            } else {
                state_changed = false;
            }
        }
    } while(true);
}

//******************************************************************************
/**
 * @brief Setup the LED task
 * 
 * This function sets up the LED task.
 * 
 * @param led_bar_number 
 * @param gpio_pin 
 * @param spinum 
 * @return esp_err_t 
 */
esp_err_t LedTaskSpi::setup(int led_bar_number, int gpio_pin, spi_host_device_t spinum, int led_count)
{
    esp_err_t ret = ESP_OK;

    this->led_bar_number = led_bar_number;
    this->gpio_pin = gpio_pin;
    this->led_count = led_count;
    this->led_pixels = (uint8_t *)malloc(this->led_count * 3);

    ESP_GOTO_ON_FALSE(
        this->led_pixels, ESP_ERR_NO_MEM, 
        err_exit, TAG, "Failed to allocate memory for LED pixels"
    );

    ESP_GOTO_ON_ERROR(
        this->rmt_over_spi.setup(spinum, gpio_pin, this->led_count), 
        err_exit, TAG, "Failed to setup RMT over SPI"
    );

    this->state_update_queue = xQueueCreate(10, sizeof(led_state_info_t));
    this->state_info.state = e_station_booting_up;

err_exit:
    return ret;
}

//******************************************************************************
/**
 * @brief Start the LED task
 * 
 * This function starts the LED task freeRTOS task on core 1.
 * 
 * @return esp_err_t 
 */
esp_err_t LedTaskSpi::start(void)
{
    char name[16] = {0};
    snprintf(name, 16, "LED%d", this->led_bar_number);
    xTaskCreatePinnedToCore(
        LedTaskSpi::svTaskCodeLed, // Function to implement the task
        name,                      // Name of the task
        5000,                      // Stack size in words
        this,                      // Task input parameter
        10,                        // Priority
        &this->task_handle,        // Task handle.
        1);                        // Core 1

    return ESP_OK;
}

//******************************************************************************
/**
 * @brief Resume the LED task
 * 
 * @return esp_err_t 
 */
esp_err_t LedTaskSpi::resume(void)
{
    vTaskResume(this->task_handle);
    return ESP_OK;
}

//******************************************************************************
/**
 * @brief Suspend the LED task
 * 
 * @return esp_err_t 
 */
esp_err_t LedTaskSpi::suspend(void)
{
    vTaskSuspend(this->task_handle);
    return ESP_OK;
}

//******************************************************************************
/**
 * @brief Set the LED pattern
 * 
 * This function sets the LED pattern.  It sends a message to the LED task via
 * the state_update_queue.
 * 
 * @param pattern 
 * @param charge_percent 
 * @return esp_err_t 
 */
esp_err_t LedTaskSpi::set_pattern(led_state_t pattern, int charge_percent)
{
    led_state_info_t state_info;
    state_info.state = pattern;
    state_info.charge_percent = charge_percent;
    xQueueSend(this->state_update_queue, &state_info, portMAX_DELAY);
    return ESP_OK;

}

//******************************************************************************
/**
 * @brief Set the LED state
 * 
 * This function sets the LED state from a string.  It maps the string to the
 * led_state_t enum and call set_pattern.
 * 
 * This function is usefull when the LED state is set from a JSON string via
 * MQTT.
 * 
 * @param new_state 
 * @param charge_percent 
 * @return esp_err_t 
 */
esp_err_t LedTaskSpi::set_state(const char *new_state, int charge_percent)
{
    led_state_t state = e_station_unknown;
    #define MAP_TO_ENUM(x) if (strcmp(new_state, #x) == 0) {\
            state = e_station_##x; \
            goto mapped; \
        }
    
    MAP_TO_ENUM(available);
    MAP_TO_ENUM(waiting_for_power);
    MAP_TO_ENUM(charging);
    MAP_TO_ENUM(charging_complete);
    MAP_TO_ENUM(out_of_service);
    MAP_TO_ENUM(disable);
    MAP_TO_ENUM(booting_up);
    MAP_TO_ENUM(offline);
    MAP_TO_ENUM(reserved);
    MAP_TO_ENUM(iot_unprovisioned);
    MAP_TO_ENUM(debug_on);
    MAP_TO_ENUM(debug_off);
    MAP_TO_ENUM(unknown);
    
    if (state == e_station_unknown) {
        ESP_LOGE(TAG, "%d: Unknown state %s", this->led_bar_number, new_state);
        return ESP_FAIL;
    }

mapped:
    return this->set_pattern(state, charge_percent);
}

const char* LedTaskSpi::get_state_as_string(void) {
    switch (this->state_info.state) {
        case e_station_available:           return "available";
        case e_station_waiting_for_power:   return "waiting_for_power";
        case e_station_charging:            return "charging";
        case e_station_charging_complete:   return "charging_complete";
        case e_station_out_of_service:      return "out_of_service";
        case e_station_disable:             return "disable";
        case e_station_booting_up:          return "booting_up";
        case e_station_offline:             return "offline";
        case e_station_reserved:            return "reserved";
        case e_station_iot_unprovisioned:   return "iot_unprovisioned";
        case e_station_debug_on:            return "debug_on";
        case e_station_debug_off:           return "debug_off";
        case e_station_unknown:             return "unknown";
        default:                            return "unknown";
    }
}
