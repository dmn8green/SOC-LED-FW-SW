#include "EthConnection.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "eth_wifi";

esp_err_t EthernetConnection::initialize(esp_eth_handle_t* eth_handle) {
    this->eth_handle = eth_handle;

    ESP_ERROR_CHECK( esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &EthernetConnection::sOnEthEvent, this) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &EthernetConnection::sOnGotIp, this) );
    
    this->on();

    return ESP_OK;
}

esp_err_t EthernetConnection::on(void) {
    return esp_eth_start(eth_handle);
}

esp_err_t EthernetConnection::off(void) {
    return esp_eth_stop(eth_handle);
}

esp_err_t EthernetConnection::connect(void) {
    return ESP_OK;
}

esp_err_t EthernetConnection::disconnect(void) {
    return ESP_OK;
}


void EthernetConnection::onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

void EthernetConnection::onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    eth_event_t event = (eth_event_t) event_id;
    switch (event) {
        case ETHERNET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Up");
            break;
        case ETHERNET_EVENT_DISCONNECTED:
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
    ESP_LOGI(TAG, "Wi-Fi got ip.");
    EthernetConnection *theThis = static_cast<EthernetConnection *>(arg);
    theThis->onGotIp(event_base, event_id, event_data);
}

void EthernetConnection::sOnEthEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    EthernetConnection *theThis = static_cast<EthernetConnection *>(arg);
    theThis->onEthEvent(event_base, event_id, event_data);
}