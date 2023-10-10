//******************************************************************************
/**
 * @file NetworkConnection.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief NetworkConnectionAgent class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#include "NetworkConnection.h"
#include "Network/Utils/ethernet_init.h"

#include "esp_log.h"
#include "esp_check.h"
#include "pin_def.h"

static const char *TAG = "NetworkConnection";

//*****************************************************************************
/**
 * @brief Setup the network connection agent.
 * 
 * This function will setup the network connection agent.  It will create the
 * wifi and ethernet interfaces and connections.  It will also initialize the
 * wifi and ethernet connections.
 * 
 * @return esp_err_t 
 */
esp_err_t NetworkConnection::setup(void)
{
    esp_err_t ret = ESP_OK;

    // Configure core ethernet stuff.  It's ok if it fails, we will just
    // disable the interface.
    this->setup_ethernet_stack();
    ESP_GOTO_ON_ERROR(esp_netif_init(), err, TAG, "Failed to initialize netif");

    // Setup the wifi and ethernet connections.  This will create the interfaces
    // and the connections.  If the settings stored in nvs are valid, then the
    // connections will start accordingly.
    ESP_GOTO_ON_ERROR(setup_wifi_connection(), err, TAG, "Failed to setup wifi");
    if (this->has_ethernet_phy) {
        ESP_GOTO_ON_ERROR(setup_ethernet_connection(), err, TAG, "Failed to setup ethernet");
    }

err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Setup the ethernet stack.
 * 
 * This function will setup the ethernet stack, internal drivers etc...
 */
void NetworkConnection::setup_ethernet_stack(void) {
    this->has_ethernet_phy = eth_init(&this->eth_handle) == ESP_OK;
    if (this->has_ethernet_phy) {
        ESP_LOGI(__func__, "Ethernet initialized");
    } else {
        ESP_LOGE(__func__, "Failed to initialize ethernet");
    }
}

//*****************************************************************************
/**
 * @brief Get the connection object for the specified interface.
 * 
 * @param interface     "wifi" or "eth"
 * @return Connection*  Pointer to the connection object, must not be freed.
 */
Connection* NetworkConnection::get_connection(const char* interface) {
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
esp_err_t NetworkConnection::setup_wifi_connection(void) {
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
    this->wifi_connection->up();

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
esp_err_t NetworkConnection::setup_ethernet_connection(void) {

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
    this->ethernet_connection->up();

    return ESP_OK;
}

//*****************************************************************************
bool NetworkConnection::is_enabled(void) {
    bool is_enabled = this->wifi_connection->is_enabled();
    if (this->has_ethernet_phy) {
        is_enabled |= this->ethernet_connection->is_enabled();
    }
    return is_enabled;
}

//*****************************************************************************
bool NetworkConnection::is_configured(void) {
    bool is_configured = this->wifi_connection->is_configured();
    if (this->has_ethernet_phy) {
        is_configured |= this->ethernet_connection->is_configured();
    }
    return is_configured;
}

//*****************************************************************************
bool NetworkConnection::check_connectivity(bool thorough_check) {
    // This should check the network using ping or something.
    if (thorough_check) {
        return true;
    } else {
        // for now just check the interface state.
        bool is_connected = this->wifi_connection->is_connected();
        if (this->has_ethernet_phy) {
            is_connected |= this->ethernet_connection->is_connected();
        }
        return is_connected;
    }
    return false;
}