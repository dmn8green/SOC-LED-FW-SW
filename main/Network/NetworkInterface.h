#pragma once

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"

#include "Utils/NoCopy.h"

class NetworkInterface : public NoCopy {
public:
    NetworkInterface(esp_netif_t* netif);
    virtual ~NetworkInterface(void) {}

    esp_err_t use_dhcp(bool use);
    esp_err_t set_network_info(esp_ip4_addr_t ip, esp_ip4_addr_t netmask, esp_ip4_addr_t gateway, esp_ip4_addr_t dns);
   
    inline esp_err_t get_ip_info(esp_netif_ip_info_t& ip_info) { return esp_netif_get_ip_info(netif, &ip_info); }
    esp_err_t get_dns_info(esp_ip4_addr_t& dns);
    inline esp_netif_t* get_netif(void) { return this->netif; }
    inline uint8_t* get_mac_address() { return mac; }

private:
    esp_netif_t *netif = nullptr;
    uint8_t mac[6];
};
