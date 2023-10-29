#include "ChargePointConfig.h"

#include "esp_log.h"

#include "Utils/KeyStore.h"

static const char *TAG = "ChargePointConfig";

/* #region helper macros */
#define ESP_SET_VALUE(key, value, commit)             \
    res = store.setKeyValue(key, value, commit);      \
    if (res != ESP_OK)                                \
    {                                                 \
        ESP_LOGE(TAG, "Failed to set " key " value"); \
        return res;                                   \
    }

#define ESP_GET_STR_VALUE(key, value, length)         \
    res = store.getKeyValueAlloc(key, value, length); \
    if (res != ESP_OK)                                \
    {                                                 \
        ESP_LOGE(TAG, "Failed to get " key " value"); \
        return res;                                   \
    }

#define ESP_GET_NUM_VALUE(key, value)                 \
    res = store.getKeyValue(key, value);              \
    if (res != ESP_OK)                                \
    {                                                 \
        ESP_LOGE(TAG, "Failed to get " key " value"); \
        return res;                                   \
    }

#define FREE_MEMBER(m) \
    if (m != nullptr)  \
    {                  \
        free(m);       \
        m = nullptr;   \
    }

/* #endregion */

//******************************************************************************
/**
 * @brief Destruct a new ChargePointConfig::ChargePointConfig object
 */
ChargePointConfig::~ChargePointConfig()
{
    ESP_LOGD(TAG, "ChargePointConfig::~ChargePointConfig()");
    FREE_MEMBER(this->group_id);
    FREE_MEMBER(this->led_1_chargepoint_station_id);
    FREE_MEMBER(this->led_2_chargepoint_station_id);
}

//******************************************************************************
/**
 * @brief set the group_id
 *
 * @note remember to call save when done setting all the station ids
 */
void ChargePointConfig::set_group_id(const char *group_id)
{
    ESP_LOGD(TAG, "ChargePointConfig::set_group_id()");
    FREE_MEMBER(this->group_id);
    this->group_id = strdup(group_id);
}

//******************************************************************************
/**
 * @brief set the station id for led 1
 *
 * @note remember to call save when done setting all the station ids
 */
void ChargePointConfig::set_led_1_station_id(const char *station_id, uint8_t port_number)
{
    ESP_LOGD(TAG, "ChargePointConfig::set_station_id()");
    FREE_MEMBER(this->led_1_chargepoint_station_id);
    this->led_1_chargepoint_station_id = strdup(station_id);
    this->led_1_chargepoint_port_number = port_number;
}

//******************************************************************************
/**
 * @brief set the station id for led 2
 *
 * @note remember to call save when done setting all the station ids
 */
void ChargePointConfig::set_led_2_station_id(const char *station_id, uint8_t port_number)
{
    ESP_LOGD(TAG, "ChargePointConfig::set_station_id()");
    FREE_MEMBER(this->led_2_chargepoint_station_id);
    this->led_2_chargepoint_station_id = strdup(station_id);
    this->led_2_chargepoint_port_number = port_number;
}

//******************************************************************************
/**
 * @brief Load the chargepoint config from NVS
 *
 * @return esp_err_t
 */
esp_err_t ChargePointConfig::load(void)
{
    ESP_LOGD(TAG, "ChargePointConfig::Load()");
    size_t valueLength = 0;
    esp_err_t res = ESP_OK;

    KeyStore store;
    res = store.openKeyStore("chargepoint", e_ro);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        goto err;
    }

    ESP_LOGI(TAG, "Loading chargepoint config");

    ESP_GET_STR_VALUE("group_id", this->group_id, valueLength);
    ESP_GET_STR_VALUE("l1_cp_sid", this->led_1_chargepoint_station_id, valueLength);
    ESP_GET_STR_VALUE("l2_cp_sid", this->led_2_chargepoint_station_id, valueLength);
    ESP_GET_NUM_VALUE("l1_port_number", this->led_1_chargepoint_port_number);
    ESP_GET_NUM_VALUE("l2_port_number", this->led_2_chargepoint_port_number);

    ESP_LOGD(TAG, "group_id: %s", this->group_id);
    ESP_LOGD(TAG, "led_1_chargepoint_station_id: %s", this->led_1_chargepoint_station_id);
    ESP_LOGD(TAG, "led_2_chargepoint_station_id: %s", this->led_2_chargepoint_station_id);
    ESP_LOGD(TAG, "led_1_port_number: %d", this->led_1_chargepoint_port_number);
    ESP_LOGD(TAG, "led_2_port_number: %d", this->led_2_chargepoint_port_number);

    this->isConfigured = true;
err:
    return res;
}

//******************************************************************************
/**
 * @brief Save the chargepoint config to NVS
 *
 * @return esp_err_t
 */
esp_err_t ChargePointConfig::save(void)
{
    ESP_LOGD(TAG, "ChargePointConfig::Save()");
    esp_err_t res = ESP_OK;

    KeyStore store;
    res = store.openKeyStore("chargepoint", e_rw);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        goto err;
    }

    ESP_LOGI(TAG, "Saving chargepoint config");
    ESP_LOGD(TAG, "group_id: %s", this->group_id);
    ESP_LOGD(TAG, "led_1_chargepoint_station_id: %s", this->led_1_chargepoint_station_id);
    ESP_LOGD(TAG, "led_2_chargepoint_station_id: %s", this->led_2_chargepoint_station_id);
    ESP_LOGD(TAG, "led_1_port_number: %d", this->led_1_chargepoint_port_number);
    ESP_LOGD(TAG, "led_2_port_number: %d", this->led_2_chargepoint_port_number);

    ESP_SET_VALUE("group_id", this->group_id, false);
    if (nullptr != this->led_1_chargepoint_station_id)
    {
        ESP_SET_VALUE("l1_cp_sid", this->led_1_chargepoint_station_id, false);
        ESP_SET_VALUE("l1_port_number", this->led_1_chargepoint_port_number, false);
    }

    if (nullptr != this->led_2_chargepoint_station_id)
    {
        ESP_SET_VALUE("l2_cp_sid", this->led_2_chargepoint_station_id, false);
        ESP_SET_VALUE("l2_port_number", this->led_2_chargepoint_port_number, false);
    }

    res = store.commit();
err:
    return res;
}

//******************************************************************************
/**
 * @brief Dump the chargepoint config to the log
 *
 * @return esp_err_t
 */
esp_err_t ChargePointConfig::dump(void)
{
    ESP_LOGD(TAG, "ChargePointConfig::Dump()");
    esp_err_t res = ESP_OK;

    KeyStore store;
    res = store.openKeyStore("chargepoint", e_ro);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        goto err;
    }

    printf("    group_id: %s\n", this->group_id ? this->group_id : "none");
    printf("    led 1 is mapped to station id: %s port %d\n", this->led_1_chargepoint_station_id ? this->led_1_chargepoint_station_id : "none", this->led_1_chargepoint_port_number);
    printf("    led 2 is mapped to station id: %s port %d\n", this->led_2_chargepoint_station_id ? this->led_2_chargepoint_station_id : "none", this->led_2_chargepoint_port_number);

err:
    return res;
}

//******************************************************************************
/**
 * @brief Reset the chargepoint config
 *
 * @return esp_err_t
 */
esp_err_t ChargePointConfig::reset(void)
{
    ESP_LOGD(TAG, "ChargePointConfig::Reset()");
    esp_err_t res = ESP_OK;

    KeyStore store;
    res = store.openKeyStore("chargepoint", e_rw);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open chargepoint config store");
        goto err;
    }

    ESP_LOGI(TAG, "Resetting chargepoint config");

    ESP_SET_VALUE("group_id", "", false);
    ESP_SET_VALUE("l1_cp_sid", "", false);
    ESP_SET_VALUE("l2_cp_sid", "", false);
    ESP_SET_VALUE("l1_port_number", (uint8_t)0, false);
    ESP_SET_VALUE("l2_port_number", (uint8_t)0, false);

    res =  store.commit();
err:
    return res;
}
