#pragma once

#include "esp_err.h"

#include "driver/rmt_tx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef enum {
    e_station_available,
    e_station_unreachable,
    e_station_status_unknown,
    e_station_in_use,
    e_station_reserved,
    e_station_test_charging
} led_state_t;

typedef struct {
    led_state_t state;
    int charge_percent;
} led_state_info_t;

class LedTask {
public:
    LedTask(void) = default;
    ~LedTask(void) = default;

    // Disable copy
    LedTask& operator= (const LedTask&) = delete;
    LedTask(const LedTask&) = delete;
    
    esp_err_t setup(int led_number, int gpio_pin);
    esp_err_t start(void);
    esp_err_t resume(void);
    esp_err_t suspend(void);
    esp_err_t stop(void);
    esp_err_t teardown(void);
    esp_err_t set_pattern(int pattern);
    esp_err_t set_state(const char* new_state, int charge_percent);

protected:
    void vTaskCodeLed(void);
    static void svTaskCodeLed( void * pvParameters ) { ((LedTask*)pvParameters)->vTaskCodeLed(); }


private:
    esp_err_t show_static_color(uint32_t color);
    // esp_err_t show_pulsing_color(uint32_t color, bool& animated);
    esp_err_t show_charging_color(uint32_t color_start, uint32_t color_end, uint32_t charge_percent);

    esp_err_t write_led_value_to_strip(void);

    int led_number;
    int gpio_pin;
    rmt_channel_handle_t led_chan;
    rmt_encoder_handle_t led_encoder;
    rmt_transmit_config_t tx_config;
    TaskHandle_t task_handle;
    uint8_t* led_pixels;
    QueueHandle_t queue;
    QueueHandle_t state_update_queue;
    led_state_info_t state_info;
};
