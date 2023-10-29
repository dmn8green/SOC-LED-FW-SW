//******************************************************************************
/**
 * @file ChargePointConfig.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief ChargePointConfig class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#pragma once

#include <string.h>
#include <stdint.h>

#include "esp_err.h"

#include "Utils/NoCopy.h"

//******************************************************************************
/**
 * @brief Chargepoint Config class.
 * 
 * @note Not sure if we should keep this info in the device.  The device should
 *       be mn8 specific, station agnostic.  The translation of status from the
 *       station to mn8 status is already done in the cloud.
 * 
 *       We keep the info for now, and we'll review later when other type of
 *       stations are added.  Also since the mapping from esp32 board to charge
 *       point station is done at the device console, it is nice to have this
 *       info.   However, if we ever provisioned the ESP32 board via an iphone
 *       or other means, this would become obsolete.
 */
class ChargePointConfig : public NoCopy {
public:
    ChargePointConfig() = default;
    ~ChargePointConfig();

    // Load/Save from NVS
    esp_err_t load(void);
    esp_err_t save(void);
    esp_err_t dump(void);
    esp_err_t reset(void);

    // Setter
    void set_group_id(const char* group_id);
    inline void set_led_station_id_by_led_number(uint8_t led_number, const char* station_id, int port_number) {
        if (led_number == 1) { this->set_led_1_station_id(station_id, port_number); } 
        else if (led_number == 2) { this->set_led_2_station_id(station_id, port_number); }
    }
    void set_led_1_station_id(const char* station_id, uint8_t port_number);
    void set_led_2_station_id(const char* station_id, uint8_t port_number);

    // inline getter
    inline const char* get_group_id(void) { return this->group_id; }
    inline const char* get_led_station_id_by_led_number(uint8_t led_number, uint8_t& port_number) {
        if (led_number == 1) { return get_led_1_station_id(port_number); } 
        else if (led_number == 2) { return get_led_2_station_id(port_number); }
        else { return nullptr; }
    }
    inline const char* get_led_1_station_id(uint8_t& port_number) { port_number = led_1_chargepoint_port_number; return this->led_1_chargepoint_station_id; }
    inline const char* get_led_2_station_id(uint8_t& port_number) { port_number = led_2_chargepoint_port_number; return this->led_2_chargepoint_station_id; }

    inline bool is_configured(void) { return this->isConfigured; }

private:
    char*   group_id = nullptr;
    char*   led_1_chargepoint_station_id = nullptr;
    uint8_t led_1_chargepoint_port_number = 0;
    char*   led_2_chargepoint_station_id = nullptr;
    uint8_t led_2_chargepoint_port_number = 0;
    bool    isConfigured = false;
};