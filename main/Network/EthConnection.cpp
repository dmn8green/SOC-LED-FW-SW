#include "EthConnection.h"
#include "EthConfiguration.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "eth_wifi";

//*****************************************************************************
/**
 * @brief Initialize the ethernet connection.
 * 
 * This function will initialize the ethernet connection.
 * Takes care of registering callbacks for the different events.
 * 
 * Load the config info from flash.
 * 
 * @param eth_handle 
 * @return esp_err_t 
 */
esp_err_t EthernetConnection::initialize(esp_eth_handle_t* eth_handle) {
    esp_err_t res = ESP_OK;
    this->eth_handle = eth_handle;

    // Here read the config info from flash.
    // If there is no config info, then use DHCP.
    // If there is config info, then use it.
    res = this->configuration->load();
    if (res == ESP_OK) {
        this->useDHCP = this->configuration->is_dhcp_enabled();
        this->ipAddress = this->configuration->get_ip_address();
        this->netmask = this->configuration->get_netmask();
        this->gateway = this->configuration->get_gateway();
    } else {
        this->useDHCP = true;
    }

    ESP_ERROR_CHECK( esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &EthernetConnection::sOnEthEvent, this) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &EthernetConnection::sOnGotIp, this) );


    this->useDHCP = true;
    this->on();

    return res;
}

//*****************************************************************************
/**
 * @brief Turn on the ethernet connection.
 * 
 * @return esp_err_t 
 */
esp_err_t EthernetConnection::on(void) {
    
    esp_err_t res = esp_eth_start(eth_handle);
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Ethernet Started");
        isActive = true;
    } else {
        ESP_LOGE(TAG, "Ethernet Start Failed");
    }
    return res;
}

esp_err_t EthernetConnection::off(void) {
    isActive = false;
    return esp_eth_stop(eth_handle);
}

esp_err_t EthernetConnection::set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway) {
    return ESP_OK;
}

esp_err_t EthernetConnection::use_dhcp(bool use) {
    return ESP_OK;
}


void EthernetConnection::onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    this->ipAddress = ip_info->ip;
    this->netmask = ip_info->netmask;
    this->gateway = ip_info->gw;
    this->isConnected = true;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&this->ipAddress));
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