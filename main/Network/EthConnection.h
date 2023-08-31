#pragma once

#include "Connection.h"
#include "NetworkConfiguration.h"

// Ethernet connection
class EthernetConnection : public Connection {
public:
    EthernetConnection(NetworkInterface* interface, NetworkConfiguration* configuration) 
        : Connection(interface), configuration(configuration) {}
    virtual const char* get_name(void) override { return "eth"; };

    esp_err_t initialize(esp_eth_handle_t* eth_handle);

    virtual esp_err_t on(void) override;
    virtual esp_err_t off(void) override;

    virtual esp_err_t set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway) override;
    virtual esp_err_t use_dhcp(bool use) override;
    virtual esp_err_t set_enabled(bool enabled) override;

protected:
    static void sOnGotIp(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static void sOnEthEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);

private:
    esp_eth_handle_t* eth_handle = nullptr;
    NetworkConfiguration* configuration = nullptr;

};  // class EthernetConnection