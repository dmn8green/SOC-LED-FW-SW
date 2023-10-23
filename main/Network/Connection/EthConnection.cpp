#include "EthConnection.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "eth_connection";

esp_err_t EthernetConnection::on_initialize(void) {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "Initializing ethernet");
    ret = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &EthernetConnection::sOnEthEvent, this);
    if (ret != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to register ETH_EVENT handler");
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &EthernetConnection::sOnGotIp, this);
    if (ret != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to register IP_EVENT handler");
        return ret;
    }

    return ESP_OK;
}

//*****************************************************************************
/**
 * @brief Turn on the ethernet connection.
 * 
 * @return esp_err_t 
 */
esp_err_t EthernetConnection::on_up(void) {
    esp_err_t ret = ESP_OK;

    this->useDHCP = this->configuration->is_dhcp_enabled();
    this->interface->use_dhcp(this->useDHCP);

    if (!this->useDHCP) {
        this->interface->set_network_info(
            this->configuration->get_ip_address(), 
            this->configuration->get_netmask(), 
            this->configuration->get_gateway(),
            this->configuration->get_dns()
        );
    }

    ESP_LOGI(TAG, "Turning on ethernet");
    ESP_GOTO_ON_ERROR(esp_eth_start(eth_handle), err, TAG, "Failed to start ethernet");

err:
    return ret;
}

esp_err_t EthernetConnection::on_down(void) {
    ESP_LOGI(TAG, "Turning off ethernet");
    return esp_eth_stop(eth_handle);
}

void EthernetConnection::onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    this->isConnected = true;

    // if (event->ip_changed) {
        this->ipAddress = ip_info->ip;
        this->netmask = ip_info->netmask;
        this->gateway = ip_info->gw;
        this->interface->get_dns_info(this->dns);

        ESP_LOGI(TAG, "Ethernet Got IP Address");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&this->ipAddress));
        ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
        ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
        ESP_LOGI(TAG, "ETHDNS:" IPSTR, IP2STR(&this->dns));
        ESP_LOGI(TAG, "~~~~~~~~~~~");
    // }
}

void EthernetConnection::onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    eth_event_t event = (eth_event_t) event_id;
    switch (event) {
        case ETHERNET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Up");
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            this->isConnected = false;
            ESP_LOGI(TAG, "Ethernet Link Down");
            break;
        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet Started");
            break;
        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            break;
        default:
            ESP_LOGI(TAG, "Ethernet Unknown Event");
            break;
    }
}


void EthernetConnection::sOnGotIp(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    EthernetConnection *theThis = static_cast<EthernetConnection *>(arg);
    theThis->onGotIp(event_base, event_id, event_data);
}

void EthernetConnection::sOnEthEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    EthernetConnection *theThis = static_cast<EthernetConnection *>(arg);
    theThis->onEthEvent(event_base, event_id, event_data);
}