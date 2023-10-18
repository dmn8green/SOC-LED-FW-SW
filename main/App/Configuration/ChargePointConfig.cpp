#include "ChargePointConfig.h"

#include "esp_log.h"

#include "Utils/KeyStore.h"

static const char* TAG = "ChargePointConfig";

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

#define FREE_MEMBER(m) \
    if (m != nullptr) { \
        free(m); \
        m = nullptr; \
    }

//******************************************************************************
/**
 * @brief Destruct a new ChargePointConfig::ChargePointConfig object
 */
ChargePointConfig::~ChargePointConfig()
{
    ESP_LOGD(TAG, "ChargePointConfig::~ChargePointConfig()");
    FREE_MEMBER(this->group_id);
    FREE_MEMBER(this->station_id);
}

//******************************************************************************
/**
 * @brief set the group_id
 * 
 * @note remember to call save
 */
void ChargePointConfig::set_group_id(const char* group_id)
{
    ESP_LOGD(TAG, "ChargePointConfig::set_group_id()");
    FREE_MEMBER(this->group_id);
    this->group_id = strdup(group_id);
}

//******************************************************************************
/**
 * @brief set the station_id
 *
 * @note remember to call save
 */
void ChargePointConfig::set_station_id(const char* station_id)
{
    ESP_LOGD(TAG, "ChargePointConfig::set_station_id()");
    FREE_MEMBER(this->station_id);
    this->station_id = strdup(station_id);

}

//******************************************************************************
/**
 * @brief Load the chargepoint config from NVS
 * 
 * @return esp_err_t 
 */
esp_err_t ChargePointConfig::load(void) {
    ESP_LOGD(TAG, "ChargePointConfig::Load()");
    size_t valueLength = 0;
    esp_err_t res = ESP_OK;

    KeyStore store;
    if (store.openKeyStore("chargepoint", e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        return false;
    }
    
    ESP_LOGI(TAG, "Loading chargepoint config");

    ESP_GET_VALUE("groupId", this->group_id, valueLength);
    ESP_GET_VALUE("stationId", this->station_id, valueLength);

    ESP_LOGD(TAG, "groupId: %s", this->group_id);
    ESP_LOGD(TAG, "stationId: %s", this->station_id);

    this->isConfigured = true;
    return ESP_OK;
}

//******************************************************************************
/**
 * @brief Save the chargepoint config to NVS
 * 
 * @return esp_err_t 
 */
esp_err_t ChargePointConfig::save(void) {
    ESP_LOGD(TAG, "ChargePointConfig::Save()");
    esp_err_t res = ESP_OK;

    KeyStore store;
    if (store.openKeyStore("chargepoint", e_rw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        return false;
    }

    ESP_LOGI(TAG, "Saving chargepoint config");
    ESP_LOGD(TAG, "groupId: %s", this->group_id);
    ESP_LOGD(TAG, "stationId: %s", this->station_id);

    ESP_SET_VALUE("groupId", this->group_id, false);
    ESP_SET_VALUE("stationId", this->station_id, false);

    return store.commit();
}

//******************************************************************************
/**
 * @brief Dump the chargepoint config to the log
 * 
 * @return esp_err_t 
 */
esp_err_t ChargePointConfig::dump(void) {
    ESP_LOGD(TAG, "ChargePointConfig::Dump()");
    esp_err_t res = ESP_OK;

    KeyStore store;
    if (store.openKeyStore("chargepoint", e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        return false;
    }

    ESP_LOGI(TAG, "Dumping chargepoint config");
    ESP_LOGD(TAG, "groupId: %s", this->group_id ? this->group_id : "none");
    ESP_LOGD(TAG, "stationId: %s", this->station_id ? this->station_id : "none");

    return ESP_OK;
}

//******************************************************************************
/**
 * @brief Reset the chargepoint config
 * 
 * @return esp_err_t 
 */
esp_err_t ChargePointConfig::reset(void) {
    ESP_LOGD(TAG, "ChargePointConfig::Reset()");
    esp_err_t res = ESP_OK;

    KeyStore store;
    if (store.openKeyStore("chargepoint", e_rw) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        return false;
    }

    ESP_LOGI(TAG, "Resetting chargepoint config");

    ESP_SET_VALUE("groupId", "", false);
    ESP_SET_VALUE("stationId", "", false);

    return store.commit();
}
