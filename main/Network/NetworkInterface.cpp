#include "NetworkInterface.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "network_interface";

void print_mac(const unsigned char *mac) {
	ESP_LOGI(__func__, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

NetworkInterface::NetworkInterface(esp_netif_t* netif) : netif(netif) {
    esp_netif_get_mac(this->netif, mac);
    print_mac(mac);
}

esp_err_t NetworkInterface::use_dhcp(bool use) {
    esp_err_t ret = ESP_OK;
    esp_netif_dhcp_status_t status = ESP_NETIF_DHCP_INIT;

    ESP_GOTO_ON_ERROR(esp_netif_dhcpc_get_status(netif, &status), err, TAG, "Failed to get DHCP status err: %d", err_rc_);

    if (use && status == ESP_NETIF_DHCP_STOPPED) {
        ESP_GOTO_ON_ERROR(esp_netif_dhcpc_start(netif), err, TAG, "Failed to start DHCP");
    } else if (!use && status == ESP_NETIF_DHCP_STARTED) {
        ESP_GOTO_ON_ERROR(esp_netif_dhcpc_stop(netif), err, TAG, "Failed to stop DHCP");
    }

err:
    return ret;
}

esp_err_t NetworkInterface::set_network_info(esp_ip4_addr_t ip, esp_ip4_addr_t netmask, esp_ip4_addr_t gateway) {
    esp_err_t ret = ESP_OK;
    esp_netif_ip_info_t info;

    info.ip = ip;
    info.netmask = netmask;
    info.gw = gateway;

    ESP_GOTO_ON_ERROR(esp_netif_set_ip_info(netif, &info), err, TAG, "Failed to set IP info");

err:
    return ret;
}
