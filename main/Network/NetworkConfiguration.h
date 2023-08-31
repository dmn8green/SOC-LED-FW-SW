#pragma once

#include "esp_netif.h"

class KeyStore;
class NetworkConfiguration {
public:
    NetworkConfiguration() {};
    ~NetworkConfiguration(void) = default;

    NetworkConfiguration& operator= (const NetworkConfiguration&) = delete;
    NetworkConfiguration(const NetworkConfiguration&) = delete;

public:
    inline bool is_enabled(void) { return this->isEnabled; }
    inline bool is_dhcp_enabled(void) { return this->useDHCP; }
    inline esp_ip4_addr_t get_ip_address(void) { return this->ipAddress; }
    inline esp_ip4_addr_t get_netmask(void) { return this->netmask; }
    inline esp_ip4_addr_t get_gateway(void) { return this->gateway; }

    inline void set_enabled(bool enabled) { this->isEnabled = enabled; }
    inline void set_dhcp_enabled(bool enabled) { this->useDHCP = enabled; }
    inline void set_ip_address(esp_ip4_addr_t ip) { this->ipAddress = ip; }
    inline void set_netmask(esp_ip4_addr_t netmask) { this->netmask = netmask; }
    inline void set_gateway(esp_ip4_addr_t gateway) { this->gateway = gateway; }

    inline bool is_configured(void) { return this->isConfigured; }

    esp_err_t load(void);
    esp_err_t save(void);
    
protected:
    virtual const char * get_store_section_name(void) = 0;
    virtual esp_err_t load_extra(KeyStore& store) { return ESP_OK; };
    virtual esp_err_t save_extra(KeyStore& store) { return ESP_OK; };

protected:
    bool useDHCP = true;
    bool isEnabled = true;
    esp_ip4_addr_t ipAddress;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gateway;
    bool isConfigured = false;
};