#pragma once

#include "esp_wifi_types.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_eth.h"

#include <string>

typedef enum {
    e_rw,
    e_ro
} e_keyStoreMode;

//*****************************************************************************
/**
 * @brief KeyStore class.
 * 
 * This class is a wrapper around the nvs functions.
 * It is used to store and retrieve key/value pairs.
 * The key/value pairs are stored in flash.
 * 
 * This class is not thread safe.  Be careful when using it.
 */
class KeyStore
{
public:
    KeyStore();
    ~KeyStore(void);

    KeyStore &operator=(const KeyStore &) = delete;
    KeyStore(const KeyStore &) = delete;

public:
    esp_err_t openKeyStore(const char *sectionName, e_keyStoreMode ksMode = e_ro);
    esp_err_t getKeyValue(const char *keyName, char *value, size_t maxValueLength);
    esp_err_t getKeyValue(const char *keyName, esp_ip4_addr_t &value);
    esp_err_t getKeyValue(const char *keyName, uint16_t &value);
    esp_err_t getKeyValue(const char *keyName, bool &value);
    esp_err_t getKeyValue(const char *keyName, wifi_mode_t &value);
    esp_err_t getKeyValue(const char *keyName, wifi_auth_mode_t &value);

    esp_err_t setKeyValue(const char *keyName, const char *value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, esp_ip4_addr_t &value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, uint16_t value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, bool value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, wifi_mode_t value, bool commit = true);
    esp_err_t setKeyValue(const char *keyName, wifi_auth_mode_t value, bool commit = true);

    esp_err_t eraseKey(const char *keyName);

    esp_err_t commit(void);

protected:
private:
    nvs_handle handle = 0;
};
