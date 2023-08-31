#include "MN8App.h"

#include "Network/ethernet_init.h"

#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_vfs_fat.h"

#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>


//*****************************************************************************
static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

//*****************************************************************************
/**
 * @brief Setup the MN8App.
 * 
 * This function will setup the wifi and ethernet connections.
 * It will also setup the led strip.
 * 
 * @return esp_err_t 
 */
esp_err_t MN8App::setup(void) {
    esp_err_t err = ESP_OK;

    ESP_ERROR_CHECK(eth_init(&this->eth_handle));

    initialize_nvs();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    
    
    
    ESP_LOGI(__func__, "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
        chip_info.cores,
        (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
        (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    uint64_t _chipmacid = 0LL;
    esp_efuse_mac_get_default((uint8_t*) (&_chipmacid));
    ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid>>32));
    ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid>>16));
    ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid));

    err = setup_wifi_connection();
    if (err != ESP_OK) {
        ESP_LOGE(__func__, "Failed to setup wifi");
        return err;
    }

    err = setup_ethernet_connection();
    if (err != ESP_OK) {
        ESP_LOGE(__func__, "Failed to setup ethernet");
        return err;
    }

    // starts the ledstrip worker task.

    return ESP_OK;
}

//*****************************************************************************
// Will be called from the main loop.
// Will handle the state machine
//   Available, in use (in session), 
//   not ready (unconfigured), 
//   watch list (haven't heard from charger in 30 minutes),
//   needs service (something is broken with the charger), 
//   configuring (user is configuring the led strip controller)
void MN8App::loop(void) {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    // ESP_LOGI(__func__, "Hello world!");
}

Connection* MN8App::get_connection(const char* interface) {
    if (strcmp(interface, "wifi") == 0 || strcmp(interface, "wireless") == 0) {
        return this->wifi_connection;
    } else if (strcmp(interface, "eth") == 0 || strcmp(interface, "ethernet") == 0) {
        return this->ethernet_connection;
    } else {
        return nullptr;
    }
}


//*****************************************************************************
esp_err_t MN8App::setup_wifi_connection(void) {
    // esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();

    esp_netif_config.if_desc = "BOO";
    esp_netif_config.route_prio = 128;
    esp_netif_t* sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);

    if (!sta_netif) {
        ESP_LOGE(__func__, "Failed to create default wifi sta interface");
        return ESP_FAIL;
    }

    this->wifi_interface = new NetworkInterface(sta_netif);
    if (!this->wifi_interface) {
        ESP_LOGE(__func__, "Failed to create wifi interface");
        return ESP_FAIL;
    }

    
    ESP_LOGI(__func__, "Created wifi interface with mac address %02x:%02x:%02x:%02x:%02x:%02x",
        this->wifi_interface->get_mac_address()[0],
        this->wifi_interface->get_mac_address()[1],
        this->wifi_interface->get_mac_address()[2],
        this->wifi_interface->get_mac_address()[3],
        this->wifi_interface->get_mac_address()[4],
        this->wifi_interface->get_mac_address()[5]
    );

    this->wifi_config = new WifiConfiguration();
    this->wifi_connection = new WifiConnection(this->wifi_interface, this->wifi_config);
    if (!this->wifi_connection) {
        ESP_LOGE(__func__, "Failed to create wifi connection");
        return ESP_FAIL;
    }
    
    this->wifi_connection->initialize();

    return ESP_OK;
}

//*****************************************************************************
esp_err_t MN8App::setup_ethernet_connection(void) {
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);

    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(this->eth_handle)));
    
    this->ethernet_interface = new NetworkInterface(eth_netif);
    if (!this->ethernet_interface) {
        ESP_LOGE(__func__, "Failed to create ethernet interface");
        return ESP_FAIL;
    }

    this->ethernet_config = new EthernetConfiguration();

    this->ethernet_connection = new EthernetConnection(this->ethernet_interface, this->ethernet_config);
    if (!this->ethernet_connection) {
        ESP_LOGE(__func__, "Failed to create ethernet connection");
        return ESP_FAIL;
    }

    this->ethernet_connection->initialize(this->eth_handle);


    return ESP_OK;
}