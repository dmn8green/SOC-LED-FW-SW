#pragma once

#include "Connection.h"
#include "WifiConfiguration.h"

// Represent a connection.  It can be unconfigured/connected/disconnected/connecting/on/off
class WifiConnection : public Connection {
public:
    WifiConnection(NetworkInterface* interface, WifiConfiguration* configuration) 
        : Connection(interface, configuration) {}

    virtual const char* get_name(void) override { return "wifi"; };

    virtual esp_err_t on(void) override;
    virtual esp_err_t off(void) override;

    esp_err_t connect(void);
    esp_err_t disconnect(void);

    // virtual esp_err_t set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway) override;
    // virtual esp_err_t use_dhcp(bool use) override;
    // virtual esp_err_t set_enabled(bool enabled) override {return ESP_OK;};

    esp_err_t set_credentials(const wifi_creds_t& creds);

protected:
    static void sOnGotIp(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static void sOnWifiDisconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onWifiDisconnect(esp_event_base_t event_base, int32_t event_id, void *event_data);

    virtual esp_err_t on_initialize(void);

private:
    wifi_creds_t wifi_creds;

};  // class WifiConnection
