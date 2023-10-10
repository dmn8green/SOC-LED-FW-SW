#include "Connection.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "connection";

//*****************************************************************************
/**
 * @brief Initialize the connection.
 * 
 * Called at startup, this will load the configuration from flash and
 * bring up the interface if the configuration is enabled.
 * 
 * @return esp_err_t 
 */
esp_err_t Connection::initialize(void) {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(__func__, "Initializing connection %s", this->get_name());

    this->isEnabled = false;
    this->useDHCP = true;
    this->isConnected = false;

    // Here read the config info from flash.
    // If there is no config info, then use DHCP.
    ret = this->configuration->load();
    // if (ret != ESP_OK || this->configuration->is_configured()) {
    // } else {
    // }

    // Let realized classes do their unique initialization.
    esp_err_t err = this->on_initialize();
    if (err != ESP_OK) {
        ESP_LOGE(__func__, "Failed to register callbacks");
        return err;
    }

    return ret;
}

//*****************************************************************************
/**
 * @brief Reset the interface config.
 * 
 * This erases the configuration from flash and resets the interface to the
 * default which is disabled, dhcp enabled.
 * 
 * @return esp_err_t 
 */
esp_err_t Connection::reset_config(void) {
    esp_err_t ret = ESP_OK;
    if (this->is_connected()) { this->down(); }

    this->isEnabled = false;
    this->isConnected = false;
    this->useDHCP = true;

    this->configuration->reset_config();
    return ret;
}

//*****************************************************************************
/**
 * @brief Dump the interface config to the console.
 * 
 * This will dump the configuration to the console using printf.
 * 
 * @return esp_err_t 
 */
esp_err_t Connection::dump_config(void) {
    return this->configuration->dump_config();
}

//*****************************************************************************
/**
 * @brief Dump the interface info to the console.
 * 
 * This will dump the interface info to the console using printf.
 * 
 * @return esp_err_t 
 */
esp_err_t Connection::dump_connection_info(void) {
    printf("  %-20s: %s\n", "Interface", this->get_name());
    printf("  %-20s: %s\n", "Is Up", this->is_enabled() ? "Yes" : "No");

    if (this->is_enabled()) {
        printf("\nCurrent this state:\n");
        printf("  %-20s: %s\n", "Connected", this->is_connected() ? "Yes" : "No");
        printf("  %-20s: %s\n", "DHCP", this->is_dhcp() ? "Yes" : "No");
    }

    if (this->is_connected()) {
        esp_netif_ip_info_t ip_info;
        esp_err_t res = this->interface->get_ip_info(ip_info);

        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get IP info");
            return res;
        }

        esp_ip4_addr_t dns;
        this->interface->get_dns_info(dns);
        
        printf("  %-20s: " IPSTR "\n", "IP Address", IP2STR(&ip_info.ip));
        printf("  %-20s: " IPSTR "\n", "Netmask", IP2STR(&ip_info.netmask));
        printf("  %-20s: " IPSTR "\n", "Gateway", IP2STR(&ip_info.gw));
        printf("  %-20s: " IPSTR "\n", "DNS", IP2STR(&dns));

        this->on_dump_connection_info();
    }
    return ESP_OK;
}

//*****************************************************************************
/**
 * @brief Set the network info.
 * 
 * This will take down the interface, set the network info, and bring the
 * interface back up.
 * 
 * @param ip 
 * @param netmask 
 * @param gateway 
 * @return esp_err_t 
 */
esp_err_t Connection::set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway, uint32_t dns) {
    esp_err_t ret = ESP_OK;
    if (this->is_connected()) { this->down(); }

    esp_ip4_addr_t eip = {ip};
    esp_ip4_addr_t enetmask = {netmask};
    esp_ip4_addr_t egateway = {gateway};
    esp_ip4_addr_t edns = {dns};

    this->configuration->set_ip_address(eip);
    this->configuration->set_netmask(enetmask);
    this->configuration->set_gateway(egateway);
    this->configuration->set_dns(edns);
    this->configuration->set_dhcp_enabled(false);

    ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
    return this->up();
err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Configure to use DHCP or not.
 * 
 * This will take down the interface, set the DHCP flag, and bring the
 * interface back up.
 * 
 * @param use 
 * @return esp_err_t 
 */
esp_err_t Connection::use_dhcp(bool use) {
    esp_err_t ret = ESP_OK;
    if (this->is_connected()) { this->down(); }

    this->configuration->set_dhcp_enabled(use);
    ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
    return this->up();
err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Turn on the interface.
 * 
 * This will bring the interface up.  If the interface is already up, then
 * this will return an error.
 * 
 * It will also save the enable to flash so it is on when rebooting.
 * 
 * @param persistent  To make the interface persistent across boot, set this to true.
 * 
 * @return esp_err_t 
 */
esp_err_t Connection::up(bool persistent) {
    esp_err_t ret = ESP_OK;

    if (this->is_enabled()) {
        ESP_LOGE(TAG, "Interface is already up");
        return ESP_ERR_INVALID_STATE;
    }

    this->isEnabled = true;
    if (persistent) {
        this->configuration->set_enabled(true);
        ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
    }

    ESP_GOTO_ON_ERROR(this->on_up(), err, TAG, "Failed to bring interface up");

err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Turn off the interface.
 * 
 * This will take the interface down.  If the interface is already down, then
 * this will return an error.
 * 
 * It will also save the enable to flash so it is off when rebooting.
 * 
 * @param persistent  To make the interface persistent across boot, set this to true.
 * 
 * @return esp_err_t 
 */
esp_err_t Connection::down(bool persistent) {
    esp_err_t ret = ESP_OK;

    if (!this->is_enabled()) {
        ESP_LOGE(TAG, "Interface is already down");
        return ESP_ERR_INVALID_STATE;
    }

    this->isEnabled = false;
    this->isConnected = false;

    if (persistent) {
        this->configuration->set_enabled(false);
        ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
    }

    ESP_GOTO_ON_ERROR(this->on_down(), err, TAG, "Failed to bring interface down");

err:
    return ret;
}
