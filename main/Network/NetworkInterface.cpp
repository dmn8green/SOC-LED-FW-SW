#include "NetworkInterface.h"

#include "esp_log.h"

void print_mac(const unsigned char *mac) {
	ESP_LOGI(__func__, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

NetworkInterface::NetworkInterface(esp_netif_t* netif) : netif(netif) {
    ESP_LOGI(__func__, "oif is %p", this->netif);
    esp_netif_get_mac(this->netif, mac);
    print_mac(mac);

    esp_wifi_get_mac(WIFI_IF_STA, mac);
    print_mac(mac);
}




