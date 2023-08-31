#include "NetworkConfiguration.h"

#include "Utils/KeyStore.h"

#include "esp_log.h"

static const char *TAG = "NetworkConfiguration";

//******************************************************************************
/**
 * @brief Macro to get values from the key store.
 * 
 * @param key
 * @param value
 * 
 * @return bool false on error.  On success is goes through.
 */
#define ESP_GET_VALUE(key, value) \
    res = store.getKeyValue(key, value); \
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
esp_err_t NetworkConfiguration::load() {
    KeyStore store;
    esp_err_t res;

    if (store.openKeyStore(this->get_store_section_name(), e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open eth config store");
        return false;
    }

    ESP_GET_VALUE("enabled", this->isEnabled);
    ESP_GET_VALUE("dhcp", this->useDHCP);
    ESP_GET_VALUE("ip", this->ipAddress);
    ESP_GET_VALUE("netmask", this->netmask);
    ESP_GET_VALUE("gateway", this->gateway);
    
    return ESP_OK;
}

esp_err_t NetworkConfiguration::save() {
    KeyStore store;
    esp_err_t res;

    if (store.openKeyStore(this->get_store_section_name(), e_rw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open eth config store");
        return false;
    }

    // No commit until we are all done.
    ESP_SET_VALUE("enabled", this->isEnabled, false);
    ESP_SET_VALUE("dhcp", this->useDHCP, false);
    ESP_SET_VALUE("ip", this->ipAddress, false);
    ESP_SET_VALUE("netmask", this->netmask, false);
    ESP_SET_VALUE("gateway", this->gateway, false);

    res = store.commit();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit eth config store");
        return res;
    }

    return ESP_OK;    
}