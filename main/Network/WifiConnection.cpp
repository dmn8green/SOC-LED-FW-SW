#include "WifiConnection.h"

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_check.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "mn8_wifi";

esp_err_t WifiConnection::on_initialize(void) {
    esp_err_t ret = ESP_OK;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize wifi");
        return ret;
    }

    esp_wifi_set_default_wifi_sta_handlers();
    ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &WifiConnection::sOnWifiDisconnect, this);
    if (ret != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to register WIFI_EVENT handler");
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiConnection::sOnGotIp, this);
    if (ret != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to register IP_EVENT handler");
        return ret;
    }

    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wifi storage");
        return ret;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wifi mode");
        return ret;
    }

    // is_configured flag guarantees that the ssid and password are valid and not null.
    WifiConfiguration* config = (WifiConfiguration*)this->configuration;
    if (config->is_configured()) {
        strlcpy((char *) this->wifi_creds.ssid, (const char*) config->get_ssid(), sizeof(this->wifi_creds.ssid));
        strlcpy((char *) this->wifi_creds.password, (const char*) config->get_password(), sizeof(this->wifi_creds.password));
    }

    return ret;
}

esp_err_t WifiConnection::on(void) {
    return esp_wifi_start();
}

esp_err_t WifiConnection::off(void) {
    return esp_wifi_stop();
}

esp_err_t WifiConnection::connect(void) {
    #pragma GCC diagnostic pop "-Wmissing-field-initializers" 
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    wifi_config_t wifi_config = { 0 };
    #pragma GCC diagnostic push "-Wmissing-field-initializers" 

    // wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;     // WIFI_ALL_CHANNEL_SCAN or WIFI_FAST_SCAN
    // wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL; // WIFI_CONNECT_AP_BY_SIGNAL or WIFI_CONNECT_AP_BY_SECURITY
    // wifi_config.sta.threshold.rssi = -127;
    
    strlcpy((char *) wifi_config.sta.ssid, (const char*) this->wifi_creds.ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *) wifi_config.sta.password, (const char*) this->wifi_creds.password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    return esp_wifi_connect();
}

esp_err_t WifiConnection::disconnect(void) {
    return esp_wifi_disconnect();
}

/**
 * @brief Set the credentials object
 * 
 * This function will set the credentials for the wifi connection.
 * It will leave the network if it is connected.
 * It will save the credentials to flash.
 * It will turn on the wifi connection and join the network.
 * 
 * @param creds {ssid, password}
 * @return esp_err_t   ESP_OK on success.
 */
esp_err_t WifiConnection::set_credentials(const wifi_creds_t& creds) { 
    esp_err_t ret = ESP_OK;
    WifiConfiguration* config = (WifiConfiguration*)this->configuration;

    if (this->is_connected()) { 
        this->disconnect(); 
        this->off();
    }
    
    this->wifi_creds = creds;
    ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");

    ESP_GOTO_ON_ERROR(this->on(), err, TAG, "Failed to turn on wifi");
    ESP_GOTO_ON_ERROR(this->connect(), err, TAG, "Failed to attempt to connect to wifi");

err:
    return ret;
}

void WifiConnection::onGotIp(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Interface is %s", esp_netif_get_desc(event->esp_netif));

    ESP_LOGI(TAG, "Wifi Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    // this->setCsiCb();

    // this->setConnected();
}

void WifiConnection::onWifiDisconnect(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
    //ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect... %d, %d", event_id, (uint32_t)event->reason);
    // this->lastRSSI = 0;  // Reset stored RSSI readings
    // this->lastNoise = 0; // Reset stored Noise readings
    // this->setDisconnected();
}

void WifiConnection::sOnGotIp(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi got ip.");
    WifiConnection *theThis = static_cast<WifiConnection *>(arg);
    theThis->onGotIp(event_base, event_id, event_data);
}

void WifiConnection::sOnWifiDisconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    WifiConnection *theThis = static_cast<WifiConnection *>(arg);
    theThis->onWifiDisconnect(event_base, event_id, event_data);
}