#pragma once

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"

class NetworkInterface {
public:
    NetworkInterface(esp_netif_t* netif);
    virtual ~NetworkInterface(void) {}

    NetworkInterface& operator= (const NetworkInterface*) = delete;
    NetworkInterface(const NetworkInterface*) = delete;

    esp_err_t get_ip_info(esp_netif_ip_info_t& ip_info) {
        return esp_netif_get_ip_info(netif, &ip_info);
    }

    uint8_t* get_mac_address() { return mac; }

    inline esp_netif_t* get_netif(void) { return this->netif; }


private:
    esp_netif_t *netif = nullptr;
    uint8_t mac[6];
};

// class WifiInterface : public NetworkInterface {
// public:
//     WifiInterface(const esp_netif_t* netif) : NetworkInterface(netif) {}
//     virtual ~WifiInterface(void) {}

//     virtual esp_err_t get_mac_address(uint8_t* mac) override {
//         return esp_wifi_get_mac(WIFI_IF_STA, mac);
//     }

//     // esp_err_t get_rssi(int8_t& rssi) {
//     //     return esp_wifi_sta_get_ap_info(&ap_info);
//     // }
// };

// class EthInterface : public NetworkInterface {
// public:
//     EthInterface(const esp_netif_t* netif) : NetworkInterface(netif) {}
//     virtual ~EthInterface(void) {}

// };