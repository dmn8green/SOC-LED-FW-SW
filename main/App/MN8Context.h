#pragma once

#include "Utils/NoCopy.h"

#include "App/NetworkAgent/NetworkConnectionAgent.h"
#include "App/MqttAgent/MqttAgent.h"
#include "App/Configuration/ThingConfig.h"

#include "LED/LedTaskSpi.h"

class MN8Context : NoCopy {
public:
    MN8Context(void) {};
    ~MN8Context(void) = default;

public:
    // Accessors
    inline bool is_iot_thing_provisioned(void) { return thing_config.is_configured(); } 
    inline NetworkConnectionAgent& get_network_connection_agent(void) { return this->network_connection_agent; }
    inline MqttAgent& get_mqtt_agent(void) { return this->mqtt_agent; }

    inline LedTaskSpi& get_led_task_0(void) { return this->led_task_0; }
    inline LedTaskSpi& get_led_task_1(void) { return this->led_task_1; }

    inline ThingConfig& get_thing_config(void) { return this->thing_config; }

private:
    NetworkConnectionAgent network_connection_agent;
    MqttAgent mqtt_agent;

    ThingConfig thing_config;

    LedTaskSpi led_task_0;
    LedTaskSpi led_task_1;
};
