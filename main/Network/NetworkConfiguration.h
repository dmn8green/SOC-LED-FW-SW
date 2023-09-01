#pragma once

#include "esp_netif.h"

class KeyStore;

class NetworkConfiguration {
public:
    NetworkConfiguration() {};
    ~NetworkConfiguration(void) = default;

    NetworkConfiguration& operator= (const NetworkConfiguration&) = delete;
    NetworkConfiguration(const NetworkConfiguration&) = delete;

public:
    inline bool is_enabled(void) { return this->isEnabled; }
    inline bool is_dhcp_enabled(void) { return this->useDHCP; }
    inline esp_ip4_addr_t get_ip_address(void) { return this->ipAddress; }
    inline esp_ip4_addr_t get_netmask(void) { return this->netmask; }
    inline esp_ip4_addr_t get_gateway(void) { return this->gateway; }

    inline void set_enabled(bool enabled) { this->isEnabled = enabled; }
    inline void set_dhcp_enabled(bool enabled) { this->useDHCP = enabled; }
    inline void set_ip_address(esp_ip4_addr_t ip) { this->ipAddress = ip; }
    inline void set_netmask(esp_ip4_addr_t netmask) { this->netmask = netmask; }
    inline void set_gateway(esp_ip4_addr_t gateway) { this->gateway = gateway; }

    inline bool is_configured(void) { return this->isConfigured; }

    esp_err_t dump_config(void);
    esp_err_t reset_config(void);
    esp_err_t load(void);
    esp_err_t save(void);
    
protected:
    virtual const char * get_store_section_name(void) = 0;
    virtual esp_err_t on_load(KeyStore& store) { return ESP_OK; };
    virtual esp_err_t on_save(KeyStore& store) { return ESP_OK; };
    virtual esp_err_t on_reset_config(KeyStore& store) { return ESP_OK; };
    virtual esp_err_t on_dump_config(KeyStore& store) { return ESP_OK; };

protected:
    bool useDHCP = true;
    bool isEnabled = false;
    esp_ip4_addr_t ipAddress = {0};
    esp_ip4_addr_t netmask = {0};
    esp_ip4_addr_t gateway = {0};
    bool isConfigured = false;
};

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
 * @brief Macro to set values in the key store.
 * 
 * @param key
 * @param value
 */
#define ESP_ERASE_KEY(key, commit) \
    res = store.eraseKey(key, commit); \
    ESP_LOGI(TAG, "Erasing key %s", key); \
    if (res != ESP_OK) { \
        ESP_LOGE(TAG, "Failed to erase " key " value"); \
    }
