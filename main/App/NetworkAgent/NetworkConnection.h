//******************************************************************************
/**
 * @file NetworkConnectionAgent.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief NetworkConnectionAgent class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "Utils/NoCopy.h"

#include "Network/NetworkInterface.h"
#include "Network/Connection/WifiConnection.h"
#include "Network/Configuration/WifiConfiguration.h"
#include "Network/Connection/EthConnection.h"
#include "Network/Configuration/EthConfiguration.h"

//******************************************************************************
/**
 * @brief NetworkConnection class
 * 
 * This class owns the network connections.  Both wifi and ethernet.  We threat
 * them as a single connection.
 */
class NetworkConnection : public NoCopy {
public:
    NetworkConnection(void) = default;
    virtual ~NetworkConnection(void) = default;

public:
    esp_err_t setup(void);
    esp_err_t start(void);

    // Accessors
    bool is_configured(void);
    bool is_enabled(void);
    bool check_connectivity(bool thorough_check = false);

    inline NetworkInterface*   get_wifi_interface(void) { return this->wifi_interface; }
    inline WifiConnection*     get_wifi_connection(void) { return this->wifi_connection; }

    inline NetworkInterface*   get_ethernet_interface(void) { return this->ethernet_interface; }
    inline EthernetConnection* get_ethernet_connection(void) { return this->ethernet_connection; }

    Connection* get_connection(const char* interface);

protected:
    void setup_ethernet_stack(void);
    esp_err_t setup_wifi_connection(void);
    esp_err_t setup_ethernet_connection(void);
    
private:
    bool has_ethernet_phy = false;
    esp_eth_handle_t *eth_handle;

    NetworkInterface* wifi_interface = nullptr;
    WifiConnection* wifi_connection = nullptr;
    WifiConfiguration* wifi_config = nullptr;

    NetworkInterface* ethernet_interface = nullptr;
    NetworkConfiguration* ethernet_config = nullptr;
    EthernetConnection* ethernet_connection = nullptr;
};