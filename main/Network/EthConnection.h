#pragma once

#include "Connection.h"
#include "NetworkConfiguration.h"

// Ethernet connection
class EthernetConnection : public Connection {
public:
    EthernetConnection(
        NetworkInterface* interface, 
        NetworkConfiguration* configuration,
        esp_eth_handle_t* eth_handle) 
        : Connection(interface, configuration), eth_handle(eth_handle) {}

    virtual const char* get_name(void) override { return "eth"; };

protected:
    static void sOnGotIp(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static void sOnEthEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);

    virtual esp_err_t on_initialize(void);
    virtual esp_err_t on_up(void) override;
    virtual esp_err_t on_down(void) override;


private:
    esp_eth_handle_t* eth_handle = nullptr;

};  // class EthernetConnection