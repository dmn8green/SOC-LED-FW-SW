//*****************************************************************************
/**
 * @file MN8App.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief MN8App class declaration
 * @version 0.1
 * @date 2023-08-25
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//*****************************************************************************

#pragma once

#include "App/NetworkAgent/NetworkConnectionAgent.h"
#include "Network/MQTT/MqttAgent.h"

#include "Utils/Singleton.h"
#include "Utils/NoCopy.h"
#include "App/Configuration/ThingConfig.h"
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

    // Accessors
    inline bool is_iot_thing_provisioned(void) { return thing_config.is_configured(); } 
    inline NetworkConnectionAgent& get_network_connection_agent(void) { return this->network_connection_agent; }

    inline LedTaskSpi& get_led_task_0(void) { return this->led_task_0; }
    inline LedTaskSpi& get_led_task_1(void) { return this->led_task_1; }

private:
    esp_err_t setup_and_start_led_tasks(void);

private:
    NetworkConnectionAgent network_connection_agent;
    MqttAgent mqtt_agent;

    ThingConfig thing_config;

    LedTaskSpi led_task_0;
    LedTaskSpi led_task_1;
};  // class MN8App