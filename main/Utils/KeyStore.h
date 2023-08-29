#pragma once

#include <esp_wifi_types.h>

#include <nvs_flash.h>
#include <string>

class KeyStore
{
public:
    static KeyStore* 

private:
    KeyStore();
    ~KeyStore();

    KeyStore(const KeyStore &) = delete;
    KeyStore &operator=(KeyStore &) = delete;

public:
    esp_err_t openKeyStore(const char *storeName, const char *sectionName, nvs_open_mode_t mode = NVS_READWRITE);
    esp_err_t getKeyValue(const char *keyName, char *&value, size_t &valueLength);
    esp_err_t getKeyValue(const char *keyName, std::string &value);
    esp_err_t getKeyValue(const char *keyName, IPAddress &value);
    esp_err_t getKeyValue(const char *keyName, uint16_t &value);
    esp_err_t getKeyValue(const char *keyName, bool &value);
    esp_err_t getKeyValue(const char *keyName, wifi_mode_t &value);
    esp_err_t getKeyValue(const char *keyName, wifi_auth_mode_t &value);

    esp_err_t setKeyValue(const char *keyName, const char *value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, std::string &value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, IPAddress &value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, uint16_t value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, bool value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, wifi_mode_t value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, wifi_auth_mode_t value, bool commit = true);

    esp_err_t eraseKey(const char *keyName);

    esp_err_t commit(void);

protected:
private:
    nvs_handle handle;
};
