#include "KeyStore.h"

#include "esp_log.h"

#include <memory.h>

static const char *TAG = "KeyStore";
static const char *KEYSTORE_NAME = "storage";

#define NVS_CALL_WITH_ERROR_CHECK(call) \
    err = call; \
    if (err != ESP_OK) { \
        ESP_LOGE(TAG, "%s:%d (%s): %s", __FILE__, __LINE__, __FUNCTION__, esp_err_to_name(err)); \
        return err; \
    }

//******************************************************************************
KeyStore::KeyStore()
{
}

//******************************************************************************
KeyStore::~KeyStore()
{
    if (0 != this->handle) {
        nvs_close(this->handle);
    }
}

//******************************************************************************
esp_err_t KeyStore::openKeyStore(const char *sectionName, e_keyStoreMode ksMode)
{
    esp_err_t err = ESP_OK;
    
    if (0 != this->handle) {
        nvs_close(this->handle);
        this->handle = 0;
    }

    nvs_handle handle;
    nvs_open_mode_t mode = ksMode == e_rw ? NVS_READWRITE : NVS_READONLY;

    NVS_CALL_WITH_ERROR_CHECK(nvs_flash_init_partition(KEYSTORE_NAME));
    err = nvs_open_from_partition(KEYSTORE_NAME, sectionName, mode, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // Passing NVS_READWRITE will create the partition
        ESP_LOGI(TAG, "Creating key store partition %s", sectionName);
        err = nvs_open_from_partition(KEYSTORE_NAME, sectionName, NVS_READWRITE, &handle);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open key store");
        return err;
    }

    this->handle = handle;
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::getKeyValueAlloc(const char *keyName, char *&value, size_t& valueLength)
{
    esp_err_t err = ESP_OK;
    size_t valueSize = 0;

    NVS_CALL_WITH_ERROR_CHECK(nvs_get_str(this->handle, keyName, NULL, &valueSize));

    value = (char*)malloc(valueSize+1);
    memset(value, 0, valueSize+1);
    NVS_CALL_WITH_ERROR_CHECK(nvs_get_str(handle, keyName, value, &valueSize));

    valueLength = valueSize;

    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::getKeyValue(const char *keyName, char *value, size_t maxValueLength)
{
    esp_err_t err = ESP_OK;
    size_t valueSize = 0;

    NVS_CALL_WITH_ERROR_CHECK(nvs_get_str(this->handle, keyName, NULL, &valueSize));

    if (valueSize > maxValueLength) {
        ESP_LOGE(TAG, "%s: value size %d is larger than max value length %d", keyName, valueSize, maxValueLength);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    NVS_CALL_WITH_ERROR_CHECK(nvs_get_str(handle, keyName, value, &valueSize));

    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::setKeyValue(const char *keyName, const char *value, bool commit)
{
    esp_err_t err = ESP_OK;
    NVS_CALL_WITH_ERROR_CHECK(nvs_set_str(this->handle, keyName, value));
    if (commit) { this->commit(); }

    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::getKeyValue(const char *keyName, esp_ip4_addr_t &value)
{
    esp_err_t err = ESP_OK;
    NVS_CALL_WITH_ERROR_CHECK(nvs_get_u32(handle, keyName, &value.addr));
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::setKeyValue(const char *keyName, esp_ip4_addr_t &value, bool commit)
{
    esp_err_t err = ESP_OK;
    NVS_CALL_WITH_ERROR_CHECK(nvs_set_u32(this->handle, keyName, value.addr));
    if (commit) { this->commit(); }
    return ESP_OK;
}


//******************************************************************************
esp_err_t KeyStore::getKeyValue(const char *keyName, uint16_t &value)
{
    esp_err_t err = ESP_OK;
    NVS_CALL_WITH_ERROR_CHECK(nvs_get_u16(handle, keyName, &value));
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::setKeyValue(const char *keyName, uint16_t value, bool commit)
{
    esp_err_t err = ESP_OK;
    NVS_CALL_WITH_ERROR_CHECK(nvs_set_u16(this->handle, keyName, value));
    if (commit) { this->commit(); }
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::getKeyValue(const char *keyName, bool &value)
{
    esp_err_t err = ESP_OK;
    uint16_t uv = 0;
    NVS_CALL_WITH_ERROR_CHECK(this->getKeyValue(keyName, uv));
    value = (bool)uv;
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::setKeyValue(const char *keyName, bool value, bool commit)
{
    return this->setKeyValue(keyName, (uint16_t)value, commit);
}

//******************************************************************************
esp_err_t KeyStore::getKeyValue(const char *keyName, wifi_mode_t &value)
{
    esp_err_t err = ESP_OK;
    uint16_t uv = 0;
    NVS_CALL_WITH_ERROR_CHECK(this->getKeyValue(keyName, uv));
    value = (wifi_mode_t)uv;
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::setKeyValue(const char *keyName, wifi_mode_t value, bool commit)
{
    return this->setKeyValue(keyName, (uint16_t)value, commit);
}

//******************************************************************************
esp_err_t KeyStore::getKeyValue(const char *keyName, wifi_auth_mode_t &value)
{
    esp_err_t err = ESP_OK;
    uint16_t uv = 0;
    NVS_CALL_WITH_ERROR_CHECK(this->getKeyValue(keyName, uv));
    value = (wifi_auth_mode_t)uv;
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::setKeyValue(const char *keyName, wifi_auth_mode_t value, bool commit)
{
    return this->setKeyValue(keyName, (uint16_t)value, commit);
}

//******************************************************************************
esp_err_t KeyStore::eraseKey(const char *keyName, bool commit)
{
    esp_err_t err = ESP_OK;
    NVS_CALL_WITH_ERROR_CHECK(nvs_erase_key(this->handle, keyName));
    
    if (commit) { return this->commit(); }
    return ESP_OK;
}

//******************************************************************************
esp_err_t KeyStore::commit(void)
{
    esp_err_t err = ESP_OK;
    NVS_CALL_WITH_ERROR_CHECK(nvs_commit(this->handle));
    return ESP_OK;
}
