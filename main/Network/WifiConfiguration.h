#pragma once

#include "NetworkConfiguration.h"
#include <string.h>

class WifiConfiguration : public NetworkConfiguration {
public:
    WifiConfiguration() {};
    ~WifiConfiguration(void) = default;

public:
    inline const char* get_ssid(void) { return this->ssid; }
    inline const char* get_password(void) { return this->password; }
    
    inline void set_ssid(const char* ssid) { strncpy(this->ssid, ssid, sizeof(this->ssid)); }
    inline void set_password(const char* password) { strncpy(this->password, password, sizeof(this->password)); }

protected:
    virtual const char * get_store_section_name(void) override { return "wifi"; }
    virtual esp_err_t load_extra(KeyStore& store) override;
    virtual esp_err_t save_extra(KeyStore& store) override;

private:
    char ssid[32];
    char password[64];
};