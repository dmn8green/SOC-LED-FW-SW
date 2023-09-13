#pragma once

#include "esp_err.h"

#include "driver/rmt_tx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef enum {
    e_station_available,           // green   (s)  - Available and ready to charge
    e_station_waiting_for_power,   // cyan    (s)  - Waiting for power to be available
    e_station_charging,            // blue    (p)  - Charging a vehicle
    e_station_charging_pulse,      // blue    (p)  - Charging a vehicle
    e_station_charging_complete,   // blue    (s)  - Charging complete, or preparing for vehicle communication after plugging a vehicle in 
    e_station_out_of_service,      // red     (s)  - Out of service
    e_station_disable,             // red     (s)  - Disabled
    e_station_booting_up,          // yellow  (p)  - Station booting up / Not ready yet
    e_station_offline,             // white   (s)  - Station offline
    e_station_reserved,            // orange  (s)  - Station reserved

    // Test patterns
    e_station_test_charging,

    // Error state
    e_station_status_unknown
} led_state_t;

typedef struct {
    led_state_t state;
    int charge_percent;
} led_state_info_t;

//******************************************************************************
class NoCopy {
public:
    NoCopy() = default;
    ~NoCopy() = default;

    // No copy constructor
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

//******************************************************************************
class ColorMode : public NoCopy {
public:
    virtual void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) = 0;
    inline uint32_t get_rate(void) { return this->rate; }
    inline void set_rate(uint32_t rate) { this->rate = rate; }

    virtual void reset(void) { this->rate = portMAX_DELAY; }
private:
    uint32_t rate = portMAX_DELAY;
};

//******************************************************************************
class StaticColorMode : public ColorMode {
public:
    inline void set_color(uint32_t color) { this->color = color; }
    void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) override;

private:
    uint32_t color;
};

//******************************************************************************
class ChargingColorMode : public ColorMode {
public:
    inline void set_color_start(uint32_t color_start) { this->color_start = color_start; }
    inline void set_color_end(uint32_t color_end) { this->color_end = color_end; }
    inline void set_charge_percent(uint32_t charge_percent) { this->charge_percent = charge_percent; }

    void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) override;

protected:
    uint32_t color_start;
    uint32_t color_end;
    uint32_t charge_percent;
};



//******************************************************************************
class PulsingColorMode : public ColorMode {
public:
    void reset(uint32_t h, uint32_t s, uint32_t v, uint32_t min_value = 10, uint32_t rate = 10);
    void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) override;

private:
    uint32_t hsv_color[3];
    uint32_t pulse_count;
    uint32_t min_value;
    uint32_t max_value;
    bool increasing;
};

//******************************************************************************
class ChargingColorPulseMode : public ColorMode {
public:
    void reset(uint32_t color_start, uint32_t h, uint32_t s, uint32_t v, uint32_t min_value = 10, uint32_t rate = 10);
    inline void set_charge_percent(uint32_t charge_percent) { this->charge_percent = charge_percent; }

    void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) override;

protected:
    StaticColorMode static_color_mode;
    PulsingColorMode pulsing_color_mode;
    uint32_t charge_percent;
};

//******************************************************************************
class TestChargingColorMode : public ChargingColorMode {
public:
    void refresh(uint8_t* led_pixels, int led_count, int start_pixel = 0) override;
};


//******************************************************************************
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
    esp_err_t set_state(const char* new_state, int charge_percent);

protected:
    void vTaskCodeLed(void);
    static void svTaskCodeLed( void * pvParameters ) { ((LedTask*)pvParameters)->vTaskCodeLed(); }

    esp_err_t write_led_value_to_strip(void);

private:

    int led_number;
    int gpio_pin;
    uint8_t* led_pixels;
    led_state_info_t state_info;

    rmt_channel_handle_t led_chan;
    rmt_encoder_handle_t led_encoder;
    rmt_transmit_config_t tx_config;

    TaskHandle_t task_handle;
    QueueHandle_t state_update_queue;

    ColorMode* color_mode;
    StaticColorMode static_color_mode;
    ChargingColorMode charging_color_mode;
    ChargingColorPulseMode charging_color_pulse_mode;
    PulsingColorMode pulsing_color_mode;
    TestChargingColorMode test_charging_color_mode;
};
