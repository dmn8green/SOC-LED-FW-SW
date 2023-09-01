#include "WifiConfiguration.h"

#include "Utils/KeyStore.h"

#include "esp_log.h"

#include <string.h>

static const char *TAG = "WifiConfiguration";

WifiConfiguration::WifiConfiguration() 
    : NetworkConfiguration()
{
    memset(&this->wifi_creds, 0, sizeof(this->wifi_creds));
}

//******************************************************************************
/**
 * @brief Macro to get values from the key store.
 * 
 * @param key
 * @param value
 * 
 * @return bool false on error.  On success is goes through.
 */
#define ESP_GET_VALUE_WLEN(key, value, length) \
    res = store.getKeyValue(key, value, length); \
    if (res != ESP_OK) { \
        ESP_LOGE(TAG, "Failed to get " key " value"); \
        return res; \
    }

//******************************************************************************
/**
 * @brief Called when the configuration is being reseted.
 * 
 * @param store 
 * @return esp_err_t 
 */
esp_err_t WifiConfiguration::on_reset_config(KeyStore& store) {
    esp_err_t res;

    // No commit until we are all done.
    ESP_ERASE_KEY("ssid", false);
    ESP_ERASE_KEY("password", false);

    return ESP_OK;
}

//******************************************************************************
/**
 * @brief Called when the configuration is being requested by the cli.
 * 
 * @param store 
 * @return esp_err_t 
 */
esp_err_t WifiConfiguration::on_dump_config(KeyStore& store) {
    printf("  %-20s: %s\n", "SSID", this->wifi_creds.ssid);
    printf("  %-20s: %s\n", "Password", this->wifi_creds.password);

    return ESP_OK;    
}

//******************************************************************************
/**
 * @brief Load the ethernet configuration from flash.
 * 
 * @return true   load successful
 * @return false  load failed  -- 
 */
esp_err_t WifiConfiguration::on_load(KeyStore& store) {
    esp_err_t res;
    memset(&this->wifi_creds, 0, sizeof(this->wifi_creds));

    ESP_GET_VALUE_WLEN("ssid", this->wifi_creds.ssid, sizeof(this->wifi_creds.ssid));
    ESP_GET_VALUE_WLEN("password", this->wifi_creds.password, sizeof(this->wifi_creds.password));

    return ESP_OK;
}

//******************************************************************************
/**
 * @brief Save the ethernet configuration to flash.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t WifiConfiguration::on_save(KeyStore& store) {
    esp_err_t res;

    ESP_SET_VALUE("ssid", this->wifi_creds.ssid, false);
    ESP_SET_VALUE("password", this->wifi_creds.password, false);

    return ESP_OK;    
}