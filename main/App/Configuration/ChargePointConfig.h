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
 *       be mn8 specific, charger agnostic.  The translation of status from the
 *       charger to mn8 status is already done in the cloud.
 * 
 *       We keep the info for now, and we'll review later when other type of
 *       chargers are added.  Also since the mapping from esp32 board to charge
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
    void set_station_id(const char* station_id);

    // inline getter
    inline const char* get_group_id(void) { return this->group_id; }
    inline const char* get_station_id(void) { return this->station_id; }

    inline bool is_configured(void) { return this->isConfigured; }

private:
    char *group_id = nullptr;
    char *station_id = nullptr;
    bool isConfigured = false;
};