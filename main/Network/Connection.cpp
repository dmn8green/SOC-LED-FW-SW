#include "Connection.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "connection";

esp_err_t Connection::initialize(void) {
    esp_err_t ret = ESP_OK;
    ESP_LOGI(__func__, "Initializing connection %s", this->get_name());

    // Here read the config info from flash.
    // If there is no config info, then use DHCP.
    ret = this->configuration->load();
    if (ret != ESP_OK || !this->configuration->is_configured()) {
        this->useDHCP = true;
        return ESP_OK;
    }

    this->isEnabled = this->configuration->is_enabled();
    this->isConnected = false;

    // Let realized classes do their unique initialization.
    esp_err_t err = this->on_initialize();
    if (err != ESP_OK) {
        ESP_LOGE(__func__, "Failed to register callbacks");
        return err;
    }

    if (this->isEnabled) {
        ret = this->on();
    }

    return ret;
}

esp_err_t Connection::reset_config(void) {
    esp_err_t ret = ESP_OK;
    if (this->is_connected()) { this->off(); }

    this->isEnabled = false;
    this->isConnected = false;
    this->useDHCP = true;

    this->configuration->reset_config();
    return ret;
}

esp_err_t Connection::dump_config(void) {
    return this->configuration->dump_config();
}

esp_err_t Connection::set_network_info(uint32_t ip, uint32_t netmask, uint32_t gateway) {
    esp_err_t ret = ESP_OK;
    if (this->is_connected()) { this->off(); }

    esp_ip4_addr_t eip = {ip};
    esp_ip4_addr_t enetmask = {netmask};
    esp_ip4_addr_t egateway = {gateway};

    this->configuration->set_ip_address(eip);
    this->configuration->set_netmask(enetmask);
    this->configuration->set_gateway(egateway);
    this->configuration->set_dhcp_enabled(false);

    ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
    return this->on();
err:
    return ret;
}

esp_err_t Connection::use_dhcp(bool use) {
    esp_err_t ret = ESP_OK;
    if (this->is_connected()) { this->off(); }

    this->configuration->set_dhcp_enabled(use);
    ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");
    return this->on();
err:
    return ret;
}

esp_err_t Connection::set_enabled(bool enabled) {
    esp_err_t ret = ESP_OK;
    if (this->is_connected()) { this->off(); }

    this->isEnabled = enabled;
    this->configuration->set_enabled(enabled);
    ESP_GOTO_ON_ERROR(this->configuration->save(), err, TAG, "Failed to save configuration");

    if (this->isEnabled) {
        ret = this->on();
    }

err:
    return ret;
}
