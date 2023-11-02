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
    void set_chargepoint_info(
        const char* group_id,
        const char* station_id_1, int port_number_1,
        const char* station_id_2, int port_number_2
    );

    // Getter

    // inline getter
    inline const char* get_group_id(void) { return this->group_id; }
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