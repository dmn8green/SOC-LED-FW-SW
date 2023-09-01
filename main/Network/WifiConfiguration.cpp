#include "WifiConfiguration.h"

#include "Utils/KeyStore.h"

#include "esp_log.h"

#include <string.h>

static const char *TAG = "WifiConfiguration";

//******************************************************************************
/**
 * @brief Macro to get values from the key store.
 * 
 * @param key
 * @param value
 * 
 * @return bool false on error.  On success is goes through.
 */
#define ESP_GET_VALUE(key, value, length) \
    res = store.getKeyValue(key, value, length); \
    if (res != ESP_OK) { \
        ESP_LOGE(TAG, "Failed to get " key " value"); \
        return res; \
    }

//******************************************************************************
/**
 * @brief Macro to set values in the key store.
 * 
 * @param key
 * @param value
 */
#define ESP_SET_VALUE(key, value, commit) \
    res = store.setKeyValue(key, value, commit); \
    if (res != ESP_OK) { \
        ESP_LOGE(TAG, "Failed to set " key " value"); \
        return res; \
    }

//******************************************************************************
/**
 * @brief Load the ethernet configuration from flash.
 * 
 * @return true   load successful
 * @return false  load failed  -- 
 */
esp_err_t WifiConfiguration::load_extra(KeyStore& store) {
    esp_err_t res;

    ESP_LOGI(TAG, "Loading wifi configuration");

    ESP_GET_VALUE("enabled", this->wifi_creds.ssid, sizeof(this->wifi_creds.ssid));
    ESP_GET_VALUE("dhcp", this->wifi_creds.password, sizeof(this->wifi_creds.password));

    ESP_LOGI(TAG, "Loaded wifi configuration");
    ESP_LOGI(TAG, "SSID: %s", this->wifi_creds.ssid);
    ESP_LOGI(TAG, "Password: %s", this->wifi_creds.password);
    
    return ESP_OK;
}

esp_err_t WifiConfiguration::save_extra(KeyStore& store) {
    esp_err_t res;

    // No commit until we are all done.
    ESP_SET_VALUE("enabled", this->wifi_creds.ssid, false);
    ESP_SET_VALUE("enabled", this->wifi_creds.password, false);

    return ESP_OK;    
}