#include "ThingConfig.h"

#include "esp_log.h"

#include "Utils/KeyStore.h"

static const char* TAG = "ThingConfig";

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

ThingConfig::ThingConfig()
{
    ESP_LOGD(TAG, "ThingConfig::ThingConfig()");

    this->thingName = nullptr;
    this->certificateArn = nullptr;
    this->certificateId = nullptr;
    this->certificatePem = nullptr;
    this->privateKey = nullptr;
    this->publicKey = nullptr;
    this->endpointAddress = nullptr;
}

ThingConfig::~ThingConfig()
{
    ESP_LOGD(TAG, "ThingConfig::~ThingConfig()");

    if (this->thingName != nullptr) {
        free(this->thingName);
        this->thingName = nullptr;
    }

    if (this->certificateArn != nullptr) {
        free(this->certificateArn);
        this->certificateArn = nullptr;
    }

    if (this->certificateId != nullptr) {
        free(this->certificateId);
        this->certificateId = nullptr;
    }

    if (this->certificatePem != nullptr) {
        free(this->certificatePem);
        this->certificatePem = nullptr;
    }

    if (this->privateKey != nullptr) {
        free(this->privateKey);
        this->privateKey = nullptr;
    }

    if (this->publicKey != nullptr) {
        free(this->publicKey);
        this->publicKey = nullptr;
    }

    if (this->endpointAddress != nullptr) {
        free(this->endpointAddress);
        this->endpointAddress = nullptr;
    }
}

// Setters
void ThingConfig::set_thing_name(const char* thingName)
{
    ESP_LOGD(TAG, "ThingConfig::set_thing_name()");

    if (this->thingName != nullptr) {
        free(this->thingName);
        this->thingName = nullptr;
    }

    if (thingName != nullptr) {
        this->thingName = strdup(thingName);
    }
}

void ThingConfig::set_certificate_arn(const char* certificateArn)
{
    ESP_LOGD(TAG, "ThingConfig::set_certificate_arn()");

    if (this->certificateArn != nullptr) {
        free(this->certificateArn);
        this->certificateArn = nullptr;
    }

    if (certificateArn != nullptr) {
        this->certificateArn = strdup(certificateArn);
    }
}

void ThingConfig::set_certificate_id(const char* certificateId)
{
    ESP_LOGD(TAG, "ThingConfig::set_certificate_id()");

    if (this->certificateId != nullptr) {
        free(this->certificateId);
        this->certificateId = nullptr;
    }

    if (certificateId != nullptr) {
        this->certificateId = strdup(certificateId);
    }
}

void ThingConfig::set_certificate_pem(const char* certificatePem)
{
    ESP_LOGD(TAG, "ThingConfig::set_certificate_pem()");

    if (this->certificatePem != nullptr) {
        free(this->certificatePem);
        this->certificatePem = nullptr;
    }

    if (certificatePem != nullptr) {
        this->certificatePem = strdup(certificatePem);
    }
}

void ThingConfig::set_private_key(const char* privateKey)
{
    ESP_LOGD(TAG, "ThingConfig::set_private_key()");

    if (this->privateKey != nullptr) {
        free(this->privateKey);
        this->privateKey = nullptr;
    }

    if (privateKey != nullptr) {
        this->privateKey = strdup(privateKey);
    }
}

void ThingConfig::set_public_key(const char* publicKey)
{
    ESP_LOGD(TAG, "ThingConfig::set_public_key()");

    if (this->publicKey != nullptr) {
        free(this->publicKey);
        this->publicKey = nullptr;
    }

    if (publicKey != nullptr) {
        this->publicKey = strdup(publicKey);
    }
}

void ThingConfig::set_endpoint_address(const char* endpointAddress)
{
    ESP_LOGD(TAG, "ThingConfig::set_endpoint_address()");

    if (this->endpointAddress != nullptr) {
        free(this->endpointAddress);
        this->endpointAddress = nullptr;
    }

    if (endpointAddress != nullptr) {
        this->endpointAddress = strdup(endpointAddress);
    }
}

// Load/Save from NVS
esp_err_t ThingConfig::load(void) {
    ESP_LOGD(TAG, "ThingConfig::Load()");
    size_t valueLength = 0;
    esp_err_t res = ESP_OK;

    KeyStore store;
    if (store.openKeyStore("iot", e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open iot config store");
        return false;
    }
    
    ESP_LOGI(TAG, "Loading thing config");

    ESP_GET_VALUE("thingName", this->thingName, valueLength);
    ESP_GET_VALUE("certificateArn", this->certificateArn, valueLength);
    ESP_GET_VALUE("certificateId", this->certificateId, valueLength);
    ESP_GET_VALUE("certificatePem", this->certificatePem, valueLength);
    ESP_GET_VALUE("privateKey", this->privateKey, valueLength);
    ESP_GET_VALUE("publicKey", this->publicKey, valueLength);
    ESP_GET_VALUE("endpointAddress", this->endpointAddress, valueLength);

    ESP_LOGD(TAG, "thingName: %s", this->thingName);
    ESP_LOGD(TAG, "certificateArn: %s", this->certificateArn);
    ESP_LOGD(TAG, "certificateId: %s", this->certificateId);
    ESP_LOGD(TAG, "certificatePem: %s", this->certificatePem);
    ESP_LOGD(TAG, "privateKey: %s", this->privateKey);
    ESP_LOGD(TAG, "publicKey: %s", this->publicKey);
    ESP_LOGD(TAG, "endpointAddress: %s", this->endpointAddress);

    this->isConfigured = true;
    return ESP_OK;
}

esp_err_t ThingConfig::save(void) {
    ESP_LOGD(TAG, "ThingConfig::Save()");
    esp_err_t res = ESP_OK;

    KeyStore store;
    if (store.openKeyStore("iot", e_ro) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open iot config store");
        return false;
    }

    ESP_LOGD(TAG, "Saving thing config");
    ESP_LOGD(TAG, "thingName: %s", this->thingName);
    ESP_LOGD(TAG, "certificateArn: %s", this->certificateArn);
    ESP_LOGD(TAG, "certificateId: %s", this->certificateId);
    ESP_LOGD(TAG, "certificatePem: %s", this->certificatePem);
    ESP_LOGD(TAG, "privateKey: %s", this->privateKey);
    ESP_LOGD(TAG, "publicKey: %s", this->publicKey);
    ESP_LOGD(TAG, "endpointAddress: %s", this->endpointAddress);

    ESP_SET_VALUE("thingName", this->thingName, false);
    ESP_SET_VALUE("certificateArn", this->certificateArn, false);
    ESP_SET_VALUE("certificateId", this->certificateId, false);
    ESP_SET_VALUE("certificatePem", this->certificatePem, false);
    ESP_SET_VALUE("privateKey", this->privateKey, false);
    ESP_SET_VALUE("publicKey", this->publicKey, false);
    ESP_SET_VALUE("endpointAddress", this->endpointAddress, false);
    


    return ESP_OK;
}


