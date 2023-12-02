#pragma once

#include "Utils/NoCopy.h"

#include "App/NetworkAgent/NetworkConnectionAgent.h"
#include "App/MqttAgent/MqttAgent.h"
#include "App/MqttAgent/IotThing.h"
#include "App/Configuration/ThingConfig.h"
#include "App/IotHeartbeat.h"

#include "LED/LedTaskSpi.h"

class MN8Context : NoCopy {
public:
    MN8Context(void) {};
    ~MN8Context(void) = default;

public:
    esp_err_t setup(void);

    // Accessors
    inline bool is_iot_thing_provisioned(void) { return thing_config.is_configured(); } 
    inline NetworkConnectionAgent& get_network_connection_agent(void) { return this->network_connection_agent; }
    inline MqttAgent& get_mqtt_agent(void) { return this->mqtt_agent; }

    inline LedTaskSpi& get_led_task_0(void) { return this->led_task_0; }
    inline LedTaskSpi& get_led_task_1(void) { return this->led_task_1; }

    inline ThingConfig& get_thing_config(void) { return this->thing_config; }

    inline IotHeartbeat& get_iot_heartbeat(void) { return this->iot_heartbeat; }
    inline IotThing& get_iot_thing(void) { return this->iot_thing; }

    inline bool is_night_mode(void) { return this->night_mode; }
    inline void set_night_mode(bool night_mode) { this->night_mode = night_mode; }

    inline bool has_night_sensor(void) { return this->night_sensor_present; }
    inline void set_has_night_sensor(bool has_night_sensor) { this->night_sensor_present = has_night_sensor; }

    inline const char* get_mac_address(void) { return this->mac_address; }

private:
    NetworkConnectionAgent network_connection_agent;
    MqttAgent mqtt_agent;

    ThingConfig thing_config;

    LedTaskSpi led_task_0;
    LedTaskSpi led_task_1;

    IotHeartbeat iot_heartbeat;

    IotThing iot_thing;

    char mac_address[13] = {0};

    bool night_mode = false;
    bool night_sensor_present = false;
};
