//******************************************************************************
/**
 * @file SiteConfig.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief SiteConfig class declaration
 * @version 0.1
 * @date 2023-12-16
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "Utils/NoCopy.h"
#include "LED/Led.h"

#include <stdint.h>

#include "esp_err.h"

//******************************************************************************
/**
 * @brief Site configuration
 * 
 * This class defines the site configuration.
 * 
 * Currently only contains the led count.  In future will contains the desired
 * colors.
 */
class SiteConfig : public NoCopy {
public:
    SiteConfig() = default;
    ~SiteConfig();

    // Load/Save from NVS
    esp_err_t load(void);
    esp_err_t save(void);
    esp_err_t dump(void);
    esp_err_t reset(void);

    // Getters
    inline const char* get_site_name(void) const { return site_name; }
    inline uint8_t get_led_length(void) const { return led_length; }
    inline bool is_configured(void) { return this->isConfigured; }

    // Setters
    void set_site_name(const char* name);
    inline void set_led_length(uint8_t count) { led_length = count; }

private:
    char *site_name = nullptr;
    uint8_t led_length = LED_FULL_SIZE;
    bool isConfigured = false;
};