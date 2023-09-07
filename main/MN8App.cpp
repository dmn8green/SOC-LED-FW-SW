#include "MN8App.h"

#include "pin_def.h"
#include "Network/Utils/ethernet_init.h"

#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_vfs_fat.h"
#include "esp_check.h"

#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

// Will be called from the main loop.
// Will handle the state machine
//   Available, in use (in session), 
//   not ready (unconfigured), 
//   watch list (haven't heard from charger in 30 minutes),
//   needs service (something is broken with the charger), 
//   configuring (user is configuring the led strip controller)

const char* TAG = "MN8App";

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
    esp_err_t ret = ESP_OK;

    ESP_ERROR_CHECK(eth_init(&this->eth_handle));

    initialize_nvs();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    thing_config.load();

    this->led_task_0.setup(0, RMT_LED_STRIP0_GPIO_NUM);
    this->led_task_1.setup(1, RMT_LED_STRIP1_GPIO_NUM);

    this->led_task_0.start();
    this->led_task_1.start();

    // ESP_LOGI(__func__, "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
    //     chip_info.cores,
    //     (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
    //     (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    // uint64_t _chipmacid = 0LL;
    // esp_efuse_mac_get_default((uint8_t*) (&_chipmacid));
    // ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid>>32));
    // ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid>>16));
    // ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid));

    ESP_GOTO_ON_ERROR(setup_wifi_connection(), err, TAG, "Failed to setup wifi");
    ESP_GOTO_ON_ERROR(setup_ethernet_connection(), err, TAG, "Failed to setup ethernet");

    // start the state machine to monitor the network connections, incoming
    // mqtt messages.
    
    // start the ledstrip worker task.

err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Loop function for the MN8App.
 * 
 * This function will be called from the main loop.
 * It will handle the state machine logic?? TBD
 * 
 */
void MN8App::loop(void) {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    // ESP_LOGI(__func__, "Hello world!");
}

//*****************************************************************************
/**
 * @brief Get the connection object for the specified interface.
 * 
 * @param interface     "wifi" or "eth"
 * @return Connection*  Pointer to the connection object, must not be freed.
 */
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
/**
 * @brief Setup the wifi connection.
 * 
 * This function will create the wifi interface and wifi connection objects.
 * It will also initialize the wifi connection.
 * 
 * @note If the wifi is configured and enabled, then the connection will be
 *       established as soon as this function returns.
 * 
 * @return esp_err_t 
 */
esp_err_t MN8App::setup_wifi_connection(void) {
    // esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
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
/**
 * @brief Setup the ethernet connection.
 * 
 * This function will create the ethernet interface and ethernet connection
 * objects.  It will also initialize the ethernet connection.
 * 
 * @note If the ethernet cable is plugged in, and it is configured and enabled,
 *       then the connection will be established as soon as this function
 *       returns.
 * 
 * @return esp_err_t 
 */
esp_err_t MN8App::setup_ethernet_connection(void) {

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();

    esp_netif_config_t cfg = {
        .base = &esp_netif_config,
        .driver = NULL,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH,
    };
    
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    ESP_LOGI(__func__, "if %p", eth_netif);

    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(this->eth_handle)));
    ESP_LOGI(__func__, "if %p", eth_netif);

    this->ethernet_interface = new NetworkInterface(eth_netif);
    if (!this->ethernet_interface) {
        ESP_LOGE(__func__, "Failed to create ethernet interface");
        return ESP_FAIL;
    }
    ESP_LOGI(__func__, "oif is %p", this->ethernet_interface);
    ESP_LOGI(__func__, "netif is %p", this->ethernet_interface->get_netif());


    this->ethernet_config = new EthernetConfiguration();
    this->ethernet_connection = new EthernetConnection(this->ethernet_interface, this->ethernet_config, this->eth_handle);
    ESP_LOGI(__func__, "st is %p", this->ethernet_connection);
    if (!this->ethernet_connection) {
        ESP_LOGE(__func__, "Failed to create ethernet connection");
        return ESP_FAIL;
    }

    this->ethernet_connection->initialize();


    return ESP_OK;
}