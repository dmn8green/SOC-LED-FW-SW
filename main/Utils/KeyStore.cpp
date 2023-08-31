#include "KeyStore.h"

#include "esp_log.h"

static const char *TAG = "KeyStore";
static const char *KEYSTORE_NAME = "storage";

#define NVS_CALL_WITH_ERROR_CHECK(call) \
    { \
        esp_err_t __err_rc = call; \
        if (__err_rc != ESP_OK) { \
            ESP_LOGE(TAG, "%s:%d (%s): %s", __FILE__, __LINE__, __FUNCTION__, esp_err_to_name(__err_rc)); \
            return __err_rc; \
        } \
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
    if (0 != this->handle) {
        nvs_close(this->handle);
        this->handle = 0;
    }

    nvs_handle handle;
    nvs_open_mode_t mode = ksMode == e_rw ? NVS_READWRITE : NVS_READONLY;

    NVS_CALL_WITH_ERROR_CHECK(nvs_flash_init_partition(KEYSTORE_NAME));
    NVS_CALL_WITH_ERROR_CHECK(nvs_open_from_partition(KEYSTORE_NAME, sectionName, mode, &handle));

    this->handle = handle;
    return ESP_OK;
}

esp_err_t KeyStore::getKeyValue(const char *keyName, char *value, size_t maxValueLength)
{
    size_t valueSize = 0;
    esp_err_t err = ESP_OK;

    err = nvs_get_str(this->handle, keyName, NULL, &valueSize);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: failed %s %s", __FUNCTION__, keyName, esp_err_to_name(err));
        return err;
    }

    if (valueSize > maxValueLength) {
        ESP_LOGE(TAG, "%s: value size %d is larger than max value length %d", __FUNCTION__, valueSize, maxValueLength);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    err = nvs_get_str(handle, keyName, value, &valueSize);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to read %s %s", keyName, esp_err_to_name(err));
        return err;
    }

    return err;
}

esp_err_t KeyStore::setKeyValue(const char *keyName, const char *value, bool commit)
{
    esp_err_t err = ESP_OK;

    err = nvs_set_str(this->handle, keyName, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to store value %s %s", keyName, esp_err_to_name(err));
        return err;
    }

    if (commit) { this->commit(); }

    return err;
}

esp_err_t KeyStore::getKeyValue(const char *keyName, esp_ip4_addr_t &value)
{
    esp_err_t err = ESP_OK;

    err = nvs_get_u32(handle, keyName, &value.addr);
    if (err != ESP_OK) {
        return err;
    }

    return err;
}

esp_err_t KeyStore::setKeyValue(const char *keyName, esp_ip4_addr_t &value, bool commit)
{
    esp_err_t err = ESP_OK;

    err = nvs_set_u32(this->handle, keyName, value.addr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to store value %s", esp_err_to_name(err));
        return err;
    }

    if (commit) { this->commit(); }
    return err;
}


esp_err_t KeyStore::getKeyValue(const char *keyName, uint16_t &value)
{
    esp_err_t err = ESP_OK;

    err = nvs_get_u16(handle, keyName, &value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to read %s %s", keyName, esp_err_to_name(err));
        return err;
    }

    return err;
}

esp_err_t KeyStore::setKeyValue(const char *keyName, uint16_t value, bool commit)
{
    esp_err_t err = ESP_OK;

    err = nvs_set_u16(this->handle, keyName, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to store value %s", esp_err_to_name(err));
        return err;
    }

    if (commit) { this->commit(); }

    return err;
}

esp_err_t KeyStore::getKeyValue(const char *keyName, bool &value)
{
    esp_err_t err = ESP_OK;
    uint16_t uv = 0;

    err = this->getKeyValue(keyName, uv);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to read %s %s", keyName, esp_err_to_name(err));
        return err;
    }

    value = (bool)uv;
    return err;
}

esp_err_t KeyStore::setKeyValue(const char *keyName, bool value, bool commit)
{
    return this->setKeyValue(keyName, (uint16_t)value, commit);
}

esp_err_t KeyStore::getKeyValue(const char *keyName, wifi_mode_t &value)
{
    esp_err_t err = ESP_OK;
    uint16_t uv = 0;

    err = this->getKeyValue(keyName, uv);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to read %s", esp_err_to_name(err));
        return err;
    }

    value = (wifi_mode_t)uv;
    return err;
}

esp_err_t KeyStore::setKeyValue(const char *keyName, wifi_mode_t value, bool commit)
{
    return this->setKeyValue(keyName, (uint16_t)value, commit);
}

esp_err_t KeyStore::getKeyValue(const char *keyName, wifi_auth_mode_t &value)
{
    esp_err_t err = ESP_OK;
    uint16_t uv = 0;

    err = this->getKeyValue(keyName, uv);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to read %s", esp_err_to_name(err));
        return err;
    }

    value = (wifi_auth_mode_t)uv;
    return err;
}

esp_err_t KeyStore::setKeyValue(const char *keyName, wifi_auth_mode_t value, bool commit)
{
    return this->setKeyValue(keyName, (uint16_t)value, commit);
}

esp_err_t KeyStore::eraseKey(const char *keyName)
{
    esp_err_t err = ESP_OK;
    err = nvs_erase_key(this->handle, keyName);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to erase %s with error %s", keyName, esp_err_to_name(err));
        return err;
    }

    err = nvs_commit(this->handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed commit changes %s", esp_err_to_name(err));
        return err;
    }

    return err;
}

esp_err_t KeyStore::commit(void)
{
    esp_err_t err = ESP_OK;
    err = nvs_commit(this->handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed commit changes %s", esp_err_to_name(err));
    }
    return err;
}
