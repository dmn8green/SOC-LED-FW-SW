#pragma once

#include "Utils/NoCopy.h"
#include "esp_err.h"
#include <stdbool.h>

class ThingConfig;
class MqttAgent;

class IotThing : public NoCopy {
    public:
        IotThing(void) = default;
        virtual ~IotThing(void) = default;

    public:
        esp_err_t setup(ThingConfig* thing_config, MqttAgent* mqtt_agent);
        esp_err_t send_night_mode(bool night_mode);
        esp_err_t send_heartbeat(
            const char* current_state,
            const char* led1_state,
            const char* led2_state,
            bool night_mode,
            bool has_night_sensor
        );
        esp_err_t ack_led_state_change(const char* received_payload);
        esp_err_t send_pong(
            const char* current_state,
            const char* led1_state,
            const char* led2_state,
            bool night_mode,
            bool has_night_sensor
        );

    private:
        ThingConfig* thing_config = nullptr;
        MqttAgent* mqtt_agent = nullptr;
};