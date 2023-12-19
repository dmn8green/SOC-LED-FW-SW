//******************************************************************************
/**
 * @file SiteConfig.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief SiteConfig class implementation
 * @version 0.1
 * @date 2023-12-16
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#include "SiteConfig.h"
#include "LED/Led.h"

#include "esp_log.h"

#include "Utils/KeyStore.h"

#include <string.h>

static const char* TAG = "SiteConfig";

#define ESP_SET_VALUE(key, value, commit) \
    res = store.setKeyValue(key, value, commit); \
    if (res != ESP_OK) { \
        ESP_LOGE(TAG, "Failed to set " key " value"); \
        return res; \
    }

#define ESP_GET_VALUE(key, value, length) \
    res = store.getKeyValueAlloc(key, value, length); \
    if (res != ESP_OK) { \
        ESP_LOGE(TAG, "Failed to get " key " value"); \
        return res; \
    }

#define ESP_GET_VALUE_INT_OR_DEFAULT(key, value, default_value) \
    res = store.getKeyValue(key, value); \
    if (res != ESP_OK) { \
        ESP_LOGE(TAG, "Failed to get " key " value"); \
        value = default_value; \
    }

#define FREE_MEMBER(m) \
    if (m != nullptr) { \
        free(m); \
        m = nullptr; \
    }

SiteConfig::~SiteConfig()
{
    ESP_LOGD(TAG, "SiteConfig::~SiteConfig()");

    FREE_MEMBER(this->site_name);
}

// Setters
void SiteConfig::set_site_name(const char* name)
{
    ESP_LOGD(TAG, "SiteConfig::set_site_name()");

    assert(name != nullptr);

    FREE_MEMBER(this->site_name);
    this->site_name = strdup(name);
}

// Load/Save from NVS
esp_err_t SiteConfig::load(void)
{
    ESP_LOGD(TAG, "SiteConfig::load()");

    KeyStore store;
    esp_err_t res = ESP_OK;
    size_t valueLength = 0;

    if (store.openKeyStore("site_config", e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open site config store");
        return false;
    }

    ESP_GET_VALUE("site_name", this->site_name, valueLength);
    ESP_GET_VALUE_INT_OR_DEFAULT("led_length", this->led_length, LED_FULL_SIZE);

    this->isConfigured = true;
    return res;
}

esp_err_t SiteConfig::save(void)
{
    ESP_LOGI(TAG, "SiteConfig::save()");

    KeyStore store;
    esp_err_t res = ESP_OK;

    ESP_LOGI(TAG, "Saving site config");
    if (store.openKeyStore("site_config", e_rw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open site config store");
        return false;
    }

    ESP_SET_VALUE("site_name", this->site_name ? this->site_name : "", false);
    ESP_SET_VALUE("led_length", this->led_length, false);
    return store.commit();
}

esp_err_t SiteConfig::dump(void)
{
    ESP_LOGD(TAG, "SiteConfig::dump()");

    KeyStore store;
    esp_err_t res = ESP_OK;

    if (store.openKeyStore("site_config", e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open site config store");
        return false;
    }

    ESP_LOGI(TAG, "site_name: %s", this->site_name);
    ESP_LOGI(TAG, "led_length: %d", this->led_length);

    return res;
}

esp_err_t SiteConfig::reset(void)
{
    ESP_LOGD(TAG, "SiteConfig::reset()");

    KeyStore store;
    if (store.openKeyStore("site_config", e_rw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open site config store");
        return false;
    }

    store.eraseKey("site_name");
    store.eraseKey("led_length");

    FREE_MEMBER(this->site_name);
    this->led_length = LED_STRIP_PIXEL_COUNT;
    this->isConfigured = false;

    return store.commit();
}