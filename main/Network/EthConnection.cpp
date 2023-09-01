#include "EthConnection.h"
#include "EthConfiguration.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "eth_connection";

// //*****************************************************************************
// /**
//  * @brief Initialize the ethernet connection.
//  * 
//  * This function will initialize the ethernet connection.
//  * Takes care of registering callbacks for the different events.
//  * 
//  * Load the config info from flash.
//  * 
//  * @param eth_handle 
//  * @return esp_err_t 
//  */
// esp_err_t EthernetConnection::initialize(esp_eth_handle_t* eth_handle) {
//     esp_err_t ret = ESP_OK;
//     this->eth_handle = eth_handle;

//     ESP_ERROR_CHECK( esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &EthernetConnection::sOnEthEvent, this) );
//     ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &EthernetConnection::sOnGotIp, this) );

//     // Here read the config info from flash.
//     // If there is no config info, then use DHCP.
//     ret = this->configuration->load();
//     if (ret != ESP_OK || !this->configuration->is_configured()) {
//         this->useDHCP = true;
//         return ESP_OK;
//     }

//     this->isEnabled = this->configuration->is_enabled();
//     this->isConnected = false;

//     if (this->isEnabled) {
//         ret = this->on();
//     }

//     return ret;
// }

esp_err_t EthernetConnection::on_initialize(void) {
    esp_err_t ret = ESP_OK;
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
esp_err_t EthernetConnection::on(void) {
    esp_err_t ret = ESP_OK;

    if (this->is_connected()) { 
        ESP_LOGE(TAG, "Ethernet is already connected");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Turning on ethernet");
    ESP_GOTO_ON_ERROR(esp_eth_start(eth_handle), err, TAG, "Failed to start ethernet");

    this->useDHCP = this->configuration->is_dhcp_enabled();
    this->interface->use_dhcp(this->useDHCP);

    if (!this->useDHCP) {
        this->interface->set_network_info(
            this->configuration->get_ip_address(), 
            this->configuration->get_netmask(), 
            this->configuration->get_gateway()
        );
    }

err:
    return ret;
}

esp_err_t EthernetConnection::off(void) {
    ESP_LOGI(TAG, "Turning off ethernet");
    this->isConnected = false;
    return esp_eth_stop(eth_handle);
}

// esp_err_t EthernetConnection::set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway) {
//     esp_err_t ret = ESP_OK;
//     if (this->is_connected()) { this->off(); }

//     esp_ip4_addr_t eip = {ip};
//     esp_ip4_addr_t enetmask = {netmask};
//     esp_ip4_addr_t egateway = {gateway};

//     this->configuration->set_ip_address(eip);
//     this->configuration->set_netmask(enetmask);
//     this->configuration->set_gateway(egateway);
//     this->configuration->set_dhcp_enabled(false);

//     ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
//     return this->on();
// err:
//     return ret;
// }

// esp_err_t EthernetConnection::use_dhcp(bool use) {
//     esp_err_t ret = ESP_OK;
//     if (this->is_connected()) { this->off(); }

//     this->configuration->set_dhcp_enabled(use);
//     ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
//     return this->on();
// err:
//     return ret;
// }

// esp_err_t EthernetConnection::set_enabled(bool enabled) {
//     esp_err_t ret = ESP_OK;
//     if (this->is_connected()) { this->off(); }

//     this->isEnabled = enabled;
//     this->configuration->set_enabled(enabled);
//     ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");

//     if (this->isEnabled) {
//         ret = this->on();
//     }

// err:
//     return ret;
// }

void EthernetConnection::onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    if (event->ip_changed) {
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
    EthernetConnection *theThis = static_cast<EthernetConnection *>(arg);
    theThis->onGotIp(event_base, event_id, event_data);
}

void EthernetConnection::sOnEthEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    EthernetConnection *theThis = static_cast<EthernetConnection *>(arg);
    theThis->onEthEvent(event_base, event_id, event_data);
}