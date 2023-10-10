#pragma once

#include "Network/NetworkInterface.h"
#include "Network/Configuration/NetworkConfiguration.h"

#include <stdint.h>

class Connection {
public:
    Connection(NetworkInterface* interface, NetworkConfiguration* configuration) 
        : interface(interface), configuration(configuration) {}

    virtual const char* get_name(void) = 0;

    esp_err_t reset_config(void);
    esp_err_t dump_config(void);
    virtual esp_err_t dump_connection_info(void);

    virtual esp_err_t initialize(void);
    virtual esp_err_t up(bool persistent = false);
    virtual esp_err_t down(bool persistent = false);

    inline bool is_configured() { return this->configuration->is_configured(); }
    inline bool is_connected() { return this->isConnected; }
    inline bool is_enabled() { return this->isEnabled; }
    inline bool is_dhcp() { return this->useDHCP; }

    inline esp_ip4_addr_t get_ip_address(void) { return this->ipAddress; }
    inline esp_ip4_addr_t get_netmask(void) { return this->netmask; }
    inline esp_ip4_addr_t get_gateway(void) { return this->gateway; }
    inline esp_ip4_addr_t get_dns(void) { return this->dns; }

    virtual esp_err_t set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway, uint32_t dns);
    virtual esp_err_t use_dhcp(bool use);

protected:
    virtual esp_err_t on_initialize(void) = 0;
    virtual esp_err_t on_up(void) = 0;
    virtual esp_err_t on_down(void) = 0;
    virtual esp_err_t on_dump_connection_info(void) { return ESP_OK; }

protected:
    bool isEnabled = false;
    bool isConnected = false;
    bool useDHCP = true;

    esp_ip4_addr_t ipAddress = {0};
    esp_ip4_addr_t netmask = {0};
    esp_ip4_addr_t gateway = {0};
    esp_ip4_addr_t dns = {0};

    NetworkInterface* interface = nullptr;
    NetworkConfiguration* configuration = nullptr;
};

// Represent a connection.  It can be unconfigured/connected/disconnected/connecting/on/off