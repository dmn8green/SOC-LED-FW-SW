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

    esp_err_t load(void);
    esp_err_t save(void);

protected:
    virtual const char * get_store_section_name(void) override { return "wifi"; }

private:
    char ssid[32];
    char password[64];
};