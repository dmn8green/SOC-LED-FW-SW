#pragma once

#include "Connection.h"
#include "WifiConfiguration.h"

// Represent a connection.  It can be unconfigured/connected/disconnected/connecting/on/off
class WifiConnection : public Connection {
public:
    WifiConnection(NetworkInterface* interface, WifiConfiguration* configuration);

    virtual const char* get_name(void) override { return "wifi"; };
    esp_err_t set_credentials(const wifi_creds_t& creds);

protected:
    static void sOnGotIp(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static void sOnWifiDisconnect(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onWifiDisconnect(esp_event_base_t event_base, int32_t event_id, void *event_data);

    virtual esp_err_t on_initialize(void) override;
    virtual esp_err_t on_up(void) override;
    virtual esp_err_t on_down(void) override;

private:
    esp_err_t join(void);
    esp_err_t leave(void);

private:
    wifi_creds_t wifi_creds;

};  // class WifiConnection
