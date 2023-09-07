#pragma once

#include "esp_err.h"

#include "driver/rmt_tx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

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

protected:
    void vTaskCodeLed(void);
    static void svTaskCodeLed( void * pvParameters );


private:
    int led_number;
    int gpio_pin;
    rmt_channel_handle_t led_chan;
    rmt_encoder_handle_t led_encoder;
    rmt_transmit_config_t tx_config;
    TaskHandle_t task_handle;
    uint8_t* led_pixels;
    QueueHandle_t queue;
};

// typedef struct {
//     int led_number;
//     int gpio_pin;
//     rmt_channel_handle_t led_chan;
//     rmt_encoder_handle_t led_encoder;
//     rmt_transmit_config_t tx_config;
//     TaskHandle_t task_handle;
//     uint8_t* led_pixels;
//     QueueHandle_t queue;
// } led_task_t;

// esp_err_t initialize_led_task(int led_number, int gpio_pin, led_task_t& out_task);
// esp_err_t start_led_task(led_task_t& in_task);
// esp_err_t stop_led_task(led_task_t& in_task);
// esp_err_t deinitialize_led_task(led_task_t& in_out_task);