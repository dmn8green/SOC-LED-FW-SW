#pragma once

#include "Network/NetworkInterface.h"
#include "Network/Connection/WifiConnection.h"
#include "Network/Configuration/WifiConfiguration.h"
#include "Network/Connection/EthConnection.h"
#include "Network/Configuration/EthConfiguration.h"

#include "Network/MQTT/MqttAgent.h"

#include "Utils/Singleton.h"
#include "Utils/NoCopy.h"
#include "IOT/ThingConfig.h"
//#include "LED/LedTask.h"
#include "LED/LedTaskSpi.h"

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

public:
    esp_err_t setup(void);
    void loop(void);

    inline NetworkInterface* get_wifi_interface(void) { return this->wifi_interface; }
    inline WifiConnection* get_wifi_connection(void) { return this->wifi_connection; }

    inline NetworkInterface* get_ethernet_interface(void) { return this->ethernet_interface; }
    inline EthernetConnection* get_ethernet_connection(void) { return this->ethernet_connection; }

    Connection* get_connection(const char* interface);
    
    inline bool is_iot_thing_provisioned(void) { return thing_config.is_configured(); } 
    inline LedTaskSpi& get_led_task_0(void) { return this->led_task_0; }
    inline LedTaskSpi& get_led_task_1(void) { return this->led_task_1; }

private:
    esp_err_t setup_wifi_connection(void);
    esp_err_t setup_ethernet_connection(void);

private:
    NetworkInterface* wifi_interface = nullptr;
    WifiConnection* wifi_connection = nullptr;
    WifiConfiguration* wifi_config = nullptr;

    NetworkInterface* ethernet_interface = nullptr;
    NetworkConfiguration* ethernet_config = nullptr;
    EthernetConnection* ethernet_connection = nullptr;

    MqttAgent mqtt_agent;

    esp_eth_handle_t *eth_handle;

    ThingConfig thing_config;

    LedTaskSpi led_task_0;
    LedTaskSpi led_task_1;
};  // class MN8App