#include "NetworkConfiguration.h"

#include "Utils/KeyStore.h"

#include "esp_log.h"

static const char *TAG = "NetworkConfiguration";

//******************************************************************************
/**
 * @brief Reset the network configuration to the default values.
 * 
 * @return esp_err_t 
 */
esp_err_t NetworkConfiguration::reset_config() {
    KeyStore store;
    esp_err_t res;

    if (store.openKeyStore(this->get_store_section_name(), e_rw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open eth config store");
        return false;
    }

    ESP_ERASE_KEY("enabled", false);
    ESP_ERASE_KEY("dhcp", false);
    ESP_ERASE_KEY("ip", false);
    ESP_ERASE_KEY("netmask", false);
    ESP_ERASE_KEY("gateway", false);

    res = this->on_reset_config(store);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset extra");
        return res;
    }

    res = store.commit();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit wifi config store");
        return res;
    }

    this->isConfigured = false;
    return ESP_OK;
}

esp_err_t NetworkConfiguration::dump_config() {
    KeyStore store;

    if (store.openKeyStore(this->get_store_section_name(), e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open eth config store");
        return false;
    }

    ESP_LOGI("Network Configuration", "  %-20s: %s", "Enabled", this->isEnabled ? "true" : "false");
    ESP_LOGI("Network Configuration", "  %-20s: %s", "DHCP", this->useDHCP ? "true" : "false");
    ESP_LOGI("Network Configuration", "  %-20s: " IPSTR, "IP Address", IP2STR(&this->ipAddress));
    ESP_LOGI("Network Configuration", "  %-20s: " IPSTR, "Netmask"   , IP2STR(&this->netmask));
    ESP_LOGI("Network Configuration", "  %-20s: " IPSTR, "Gateway"   , IP2STR(&this->gateway));

    this->on_dump_config(store);
    return ESP_OK;
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

    this->isConfigured = false;
    this->useDHCP = true;
    this->isEnabled = false;
    this->ipAddress.addr = 0;
    this->netmask.addr = 0;
    this->gateway.addr = 0;

    ESP_GET_VALUE("enabled", this->isEnabled);
    ESP_GET_VALUE("dhcp", this->useDHCP);
    ESP_GET_VALUE("ip", this->ipAddress);
    ESP_GET_VALUE("netmask", this->netmask);
    ESP_GET_VALUE("gateway", this->gateway);

    ESP_LOGI(TAG, "  %-20s: " IPSTR, "IP Address", IP2STR(&this->ipAddress));
    ESP_LOGI(TAG, "  %-20s: " IPSTR, "Netmask"   , IP2STR(&this->netmask));
    ESP_LOGI(TAG, "  %-20s: " IPSTR, "Gateway"   , IP2STR(&this->gateway));

    res = this->on_load(store);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load extra");
        return res;
    }

    this->isConfigured = true;

    return ESP_OK;
}

esp_err_t NetworkConfiguration::save() {
    KeyStore store;
    esp_err_t res;

    if (store.openKeyStore(this->get_store_section_name(), e_rw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open eth config store");
        return false;
    }

    ESP_LOGI(TAG, "  %-20s: " IPSTR, "IP Address", IP2STR(&this->ipAddress));
    ESP_LOGI(TAG, "  %-20s: " IPSTR, "Netmask"   , IP2STR(&this->netmask));
    ESP_LOGI(TAG, "  %-20s: " IPSTR, "Gateway"   , IP2STR(&this->gateway));

    // No commit until we are all done.
    ESP_SET_VALUE("enabled", this->isEnabled, false);
    ESP_SET_VALUE("dhcp", this->useDHCP, false);
    ESP_SET_VALUE("ip", this->ipAddress, false);
    ESP_SET_VALUE("netmask", this->netmask, false);
    ESP_SET_VALUE("gateway", this->gateway, false);

    res = this->on_save(store);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save extra");
        return res;
    }

    res = store.commit();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit eth config store");
        return res;
    }

    // It might have already been configured.  Checking for that would add
    // unneeded extra instructions and slow things down (not that it matters)
    this->isConfigured = true;
    return ESP_OK;
}