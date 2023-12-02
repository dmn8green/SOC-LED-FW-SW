#pragma once

#include "Utils/NoCopy.h"
#include "esp_err.h"
#include <stdbool.h>

class ThingConfig;
class MqttAgent;
class ChargePointConfig;

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

        esp_err_t force_refresh_proxy(ChargePointConfig* cp_config);
        esp_err_t request_latest_from_proxy(ChargePointConfig* cp_config);
        esp_err_t register_cp_station(ChargePointConfig* cp_config);
        esp_err_t unregister_cp_station(ChargePointConfig* cp_config);

    private:
        ThingConfig* thing_config = nullptr;
        MqttAgent* mqtt_agent = nullptr;
};