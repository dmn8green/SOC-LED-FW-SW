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

#include "LedTaskSpi.h"

#include "Utils/Colors.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_rom_gpio.h"
#include "soc/spi_periph.h"
#include "hal/gpio_types.h"

#include <memory.h>
#include <math.h>

#define LED_STRIP_PIXEL_COUNT 32

static const char *TAG = "LedTaskSpi";

// Gotta love meta programming.
#define STATIC_ANIM_CASE(x, color) \
    case x: \
    ESP_LOGI(TAG, "%d: Switching to static anim state " #x " with the color " #color, this->led_number); \
    this->static_animation.reset(color); this->animation = &this->static_animation; break;


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
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Starting LED task %d", this->led_number);

    do
    {
        if (state_changed)
        {
            ESP_LOGI(TAG, "%d: State changed", this->led_number);
            ESP_LOGI(TAG, "New state is %d", this->state_info.state);
            switch (state_info.state)
            {
            STATIC_ANIM_CASE(e_station_available,           COLOR_GREEN);
            STATIC_ANIM_CASE(e_station_waiting_for_power,   COLOR_CYAN);
            STATIC_ANIM_CASE(e_station_charging_complete,   COLOR_BLUE);
            STATIC_ANIM_CASE(e_station_out_of_service,      COLOR_RED);
            STATIC_ANIM_CASE(e_station_disable,             COLOR_RED);
            STATIC_ANIM_CASE(e_station_offline,             COLOR_WHITE);
            STATIC_ANIM_CASE(e_station_reserved,            COLOR_ORANGE);
            case e_station_charging:
                ESP_LOGI(TAG, "%d: Charging", this->led_number);
                this->charging_animation.reset(COLOR_BLUE, COLOR_WHITE, COLOR_BLUE, 250);
                this->charging_animation.set_charge_percent(state_info.charge_percent);
                this->animation = &this->charging_animation;
                break;
            case e_station_booting_up:
                //ESP_LOGI(TAG, "%d: Station is booting up", this->led_number);
                this->pulsing_animation.reset(COLOR_YELLOW_HSV, 0, 20, false, &smooth_rate_pulse_curve);
                this->animation = &this->pulsing_animation;
                break;
            default:
                ESP_LOGE(TAG, "%d: Unknown state %d", this->led_number, state_info.state);
                break;
            }

            state_changed = false;

            // TODO: FIGURE OUT WHAT TO DO IF WE CAN'T CHANGE THE LED STATE
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "%d: Failed to show state %d", this->led_number, state_info.state);
            }
        }

        if (this->animation != NULL) {
            this->animation->refresh(this->led_pixels, LED_STRIP_PIXEL_COUNT);
            this->rmt_over_spi.write_led_value_to_strip(this->led_pixels);
            queue_timeout = this->animation->get_rate() / portTICK_PERIOD_MS;
            // ESP_LOGI(TAG, "%d: Queue timeout: %ld", this->led_number, queue_timeout);
        }

        if (xQueueReceive(this->state_update_queue, &updated_state, queue_timeout) == pdTRUE)
        {
            ESP_LOGI(TAG, "%d: Switching to state %d", this->led_number, updated_state.state);
            if (state_info.state != updated_state.state || state_info.charge_percent != updated_state.charge_percent)
            {
                state_info = updated_state;
                state_changed = true;
            } else {
                state_changed = false;
            }
        }
    } while(true);
}

esp_err_t LedTaskSpi::setup(int led_number, int gpio_pin, spi_host_device_t spinum)
{
    esp_err_t ret = ESP_OK;

    this->led_number = led_number;
    this->gpio_pin = gpio_pin;

    this->led_pixels = (uint8_t *)malloc(LED_STRIP_PIXEL_COUNT * 3);
    if (this->led_pixels == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for LED pixels");
        ret = ESP_ERR_NO_MEM;
        goto err_exit;
    }

    ESP_GOTO_ON_ERROR(this->rmt_over_spi.setup(spinum, gpio_pin, LED_STRIP_PIXEL_COUNT), err_exit, TAG, "Failed to setup RMT over SPI");

    this->state_update_queue = xQueueCreate(10, sizeof(led_state_info_t));
    this->state_info.state = e_station_available;//e_station_booting_up;

err_exit:
    return ret;
}

esp_err_t LedTaskSpi::start(void)
{
    char name[16] = {0};
    snprintf(name, 16, "LED%d", this->led_number);
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

esp_err_t LedTaskSpi::resume(void)
{
    vTaskResume(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTaskSpi::suspend(void)
{
    vTaskSuspend(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTaskSpi::set_pattern(led_state_t pattern, int charge_percent)
{
    led_state_info_t state_info;
    state_info.state = pattern;
    state_info.charge_percent = charge_percent;
    xQueueSend(this->state_update_queue, &state_info, portMAX_DELAY);
    return ESP_OK;

}

esp_err_t LedTaskSpi::set_state(const char *new_state, int charge_percent)
{
    led_state_t state = e_station_status_unknown;
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
    
    if (state == e_station_status_unknown) {
        ESP_LOGE(TAG, "%d: Unknown state %s", this->led_number, new_state);
        return ESP_FAIL;
    }

mapped:
    return this->set_pattern(state, charge_percent);
}