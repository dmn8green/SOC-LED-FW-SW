#pragma once

#include "NetworkConfiguration.h"
#include <string.h>

typedef struct {
    char ssid[32];                         /**< SSID of target AP. */
    char password[64];                     /**< Password of target AP. */
} wifi_creds_t;

class WifiConfiguration : public NetworkConfiguration {
public:
    WifiConfiguration();
    ~WifiConfiguration(void) = default;

public:
    inline wifi_creds_t& get_wifi_creds(void) { return this->wifi_creds; }
    inline void set_wifi_creds(const wifi_creds_t& creds) { this->wifi_creds = creds; }
    
protected:
    virtual const char * get_store_section_name(void) override { return "wifi"; }
    virtual esp_err_t on_load(KeyStore& store) override;
    virtual esp_err_t on_save(KeyStore& store) override;

    virtual esp_err_t on_reset_config(KeyStore& store) override;
    virtual esp_err_t on_dump_config(KeyStore& store) override;

private:
    wifi_creds_t wifi_creds;
};