#include "NetworkInterface.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "network_interface";

void print_mac(const unsigned char *mac) {
	ESP_LOGI(__func__, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

//*****************************************************************************
/**
 * @brief Construct a new Network Interface:: Network Interface object
 * 
 * The network interface is the lowest level of the network stack.  It is a
 * wrapper around the esp_netif_t structure.  It is used by the Connection
 * class to manage the network interface.
 * 
 * @param netif 
 */
NetworkInterface::NetworkInterface(esp_netif_t* netif) : netif(netif) {
    esp_netif_get_mac(this->netif, mac);
    print_mac(mac);
}

esp_err_t NetworkInterface::use_dhcp(bool use) {
    esp_err_t ret = ESP_OK;
    esp_netif_dhcp_status_t status = ESP_NETIF_DHCP_INIT;

    ESP_GOTO_ON_ERROR(esp_netif_dhcpc_get_status(netif, &status), err, TAG, "Failed to get DHCP status err: %d", err_rc_);

    ESP_LOGI(TAG, "NetworkInterface::use_dhcp use: %d status %d", use, status);
    if (use && status == ESP_NETIF_DHCP_STOPPED) {
        ESP_LOGI("NetworkInterface::use_dhcp", "Starting DHCP");
        ESP_GOTO_ON_ERROR(esp_netif_dhcpc_start(netif), err, TAG, "Failed to start DHCP");
    } else if (!use && status != ESP_NETIF_DHCP_STOPPED) {
        ESP_LOGI("NetworkInterface::use_dhcp", "Stopping DHCP");
        ESP_GOTO_ON_ERROR(esp_netif_dhcpc_stop(netif), err, TAG, "Failed to stop DHCP");
    }

err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Set the network info.
 * 
 * This configures the network interface with the given IP address, netmask,
 * and gateway.  It also sets the DNS server.
 * 
 * @param ip 
 * @param netmask 
 * @param gateway 
 * @return esp_err_t 
 */
esp_err_t NetworkInterface::set_network_info(esp_ip4_addr_t ip, esp_ip4_addr_t netmask, esp_ip4_addr_t gateway, esp_ip4_addr_t dns) {
    esp_err_t ret = ESP_OK;
    esp_netif_ip_info_t info;
    esp_netif_dns_info_t dns_info;

    info.ip = ip;
    info.netmask = netmask;
    info.gw = gateway;
    ESP_GOTO_ON_ERROR(esp_netif_set_ip_info(netif, &info), err, TAG, "Failed to set IP info err: %d", err_rc_);

    dns_info.ip.type = ESP_IPADDR_TYPE_V4;
    dns_info.ip.u_addr.ip4.addr = dns.addr;
    ESP_GOTO_ON_ERROR(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info), err, TAG, "Failed to set DNS info err: %d", err_rc_);

    dns_info.ip.u_addr.ip4.addr = dns.addr;
    ESP_GOTO_ON_ERROR(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info), err, TAG, "Failed to set DNS info err: %d", err_rc_);

    dns_info.ip.u_addr.ip4.addr = dns.addr;
    ESP_GOTO_ON_ERROR(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_FALLBACK, &dns_info), err, TAG, "Failed to set DNS info err: %d", err_rc_);

err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Get the DNS info.
 * 
 * @param dns 
 * @return esp_err_t 
 */
esp_err_t NetworkInterface::get_dns_info(esp_ip4_addr_t& dns) {
    esp_err_t ret = ESP_OK;
    esp_netif_dns_info_t dns_info;

    ESP_GOTO_ON_ERROR(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info), err, TAG, "Failed to get DNS info err: %d", err_rc_);
    dns = dns_info.ip.u_addr.ip4;

err:
    return ret;
}