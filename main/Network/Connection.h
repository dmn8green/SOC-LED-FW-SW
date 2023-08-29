#pragma once

#include "NetworkInterface.h"

class Connection {
public:
    Connection(NetworkInterface* interface) : interface(interface) {}

    virtual esp_err_t on(void) = 0;
    virtual esp_err_t off(void) = 0;

    virtual esp_err_t connect(void) = 0;
    virtual esp_err_t disconnect(void) = 0;

protected:
    esp_ip4_addr ipAddr;
    esp_ip4_addr gwAddr;
    esp_ip4_addr dnsAddr;
    NetworkInterface* interface;
};

// Represent a connection.  It can be unconfigured/connected/disconnected/connecting/on/off