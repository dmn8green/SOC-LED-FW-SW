#pragma once

#include "Network/NetworkInterface.h"
#include "Network/WifiConnection.h"
#include "Network/EthConnection.h"

#include "Utils/Singleton.h"

/**
 * @brief MN8App class.
 * 
 * This is the main class of the project.
 * It is a singleton class.
 * It holds the overall system logic along with the interfaces to the different
 * peripherals such as the (wifi/ethernet) connection, the led strip, etc.
 * 
 * It is responsible for the setup and the main loop.
 */
class MN8App : public Singleton<MN8App> {
public:
    MN8App(token) {};
    ~MN8App(void) = default;

    MN8App& operator= (const MN8App&) = delete;
    MN8App(const MN8App&) = delete;

public:
    esp_err_t setup(void);
    void loop(void);

    inline NetworkInterface* get_wifi_interface(void) { return this->wifi_interface; }
    inline WifiConnection* get_wifi_connection(void) { return this->wifi_connection; }

    inline NetworkInterface* get_ethernet_interface(void) { return this->ethernet_interface; }
    inline EthernetConnection* get_ethernet_connection(void) { return this->ethernet_connection; }

private:
    esp_err_t setup_wifi_connection(void);
    esp_err_t setup_ethernet_connection(void);

private:
    NetworkInterface* wifi_interface = nullptr;
    WifiConnection* wifi_connection = nullptr;

    NetworkInterface* ethernet_interface = nullptr;
    EthernetConnection* ethernet_connection = nullptr;

    esp_eth_handle_t *eth_handle;

};  // class MN8App