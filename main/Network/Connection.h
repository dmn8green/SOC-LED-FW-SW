#pragma once

#include "NetworkInterface.h"
#include "NetworkConfiguration.h"

#include <stdint.h>

class Connection {
public:
    Connection(NetworkInterface* interface, NetworkConfiguration* configuration) 
        : interface(interface), configuration(configuration) {}

    virtual const char* get_name(void) = 0;

    virtual esp_err_t initialize(void);
    virtual esp_err_t on(void) = 0;
    virtual esp_err_t off(void) = 0;

    inline bool is_connected() { return this->isConnected; }
    inline bool is_enabled() { return this->isEnabled; }
    inline bool is_dhcp() { return this->useDHCP; }

    inline esp_ip4_addr_t get_ip_address(void) { return this->ipAddress; }
    inline esp_ip4_addr_t get_netmask(void) { return this->netmask; }
    inline esp_ip4_addr_t get_gateway(void) { return this->gateway; }

    virtual esp_err_t set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway);
    virtual esp_err_t use_dhcp(bool use);
    virtual esp_err_t set_enabled(bool enabled);

protected:
    virtual esp_err_t on_initialize(void) = 0;

protected:
    bool isEnabled = false;
    bool isConnected = false;
    bool useDHCP = true;

    esp_ip4_addr_t ipAddress = {0};
    esp_ip4_addr_t netmask = {0};
    esp_ip4_addr_t gateway = {0};

    NetworkInterface* interface = nullptr;
    NetworkConfiguration* configuration = nullptr;
};

// Represent a connection.  It can be unconfigured/connected/disconnected/connecting/on/off