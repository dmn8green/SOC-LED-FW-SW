#pragma once

#include "Utils/Singleton.h"

#include "esp_netif.h"

class EthernetConfiguration : public Singleton<EthernetConfiguration> {
public:
    EthernetConfiguration(token) {};
    ~EthernetConfiguration(void) = default;

    EthernetConfiguration& operator= (const EthernetConfiguration&) = delete;
    EthernetConfiguration(const EthernetConfiguration&) = delete;

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

    esp_err_t load(void);
    esp_err_t save(void);

private:
    bool useDHCP = true;
    bool isEnabled = true;
    esp_ip4_addr_t ipAddress;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gateway;
};