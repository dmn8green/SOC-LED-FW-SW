#pragma once

#include "NetworkInterface.h"

#include <stdint.h>

class Connection {
public:
    Connection(NetworkInterface* interface) : interface(interface) {}

    virtual const char* get_name(void) = 0;

    virtual esp_err_t on(void) = 0;
    virtual esp_err_t off(void) = 0;

    inline bool is_connected() { return this->isConnected; }
    inline bool is_enabled() { return this->isEnabled; }
    inline bool is_dhcp() { return this->useDHCP; }
    inline esp_ip4_addr_t get_ip_address(void) { return this->ipAddress; }
    inline esp_ip4_addr_t get_netmask(void) { return this->netmask; }
    inline esp_ip4_addr_t get_gateway(void) { return this->gateway; }

    virtual esp_err_t set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway) = 0;
    virtual esp_err_t use_dhcp(bool use) = 0;
    virtual esp_err_t set_enabled(bool enabled) = 0;

protected:
    bool isEnabled = false;
    bool isConnected = false;
    bool useDHCP = true;
    esp_ip4_addr_t ipAddress;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gateway;
    NetworkInterface* interface;
};

// Represent a connection.  It can be unconfigured/connected/disconnected/connecting/on/off