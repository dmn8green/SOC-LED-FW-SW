#pragma once

#include "Connection.h"

// Ethernet connection
class EthernetConnection : public Connection {
public:
    EthernetConnection(NetworkInterface* interface) : Connection(interface) {}
    virtual const char* get_name(void) override { return "eth"; };

    esp_err_t initialize(esp_eth_handle_t* eth_handle);

    virtual esp_err_t on(void) override;
    virtual esp_err_t off(void) override;

    virtual esp_err_t connect(void) override;
    virtual esp_err_t disconnect(void) override;

protected:
    static void sOnGotIp(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static void sOnEthEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);

private:
    esp_eth_handle_t* eth_handle;

};  // class EthernetConnection