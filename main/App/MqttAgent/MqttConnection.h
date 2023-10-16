//******************************************************************************
/**
 * @file MqttConnection.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief MqttConnection class definition
 * @version 0.1
 * @date 2023-10-10
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "Utils/NoCopy.h"

#include "App/Configuration/ThingConfig.h"

#include "core_mqtt.h"
#include "core_mqtt_state.h"

#include "network_transport.h"
#include "backoff_algorithm.h"

#include <stdint.h>


class MqttConnection : public NoCopy {
public:
    MqttConnection(void) = default;
    virtual ~MqttConnection(void) = default;

public:
    esp_err_t initialize(ThingConfig* thing_config);
    esp_err_t connect_with_retries(MQTTContext_t* mqtt_context, uint16_t retry_count = 5);
    esp_err_t disconnect(MQTTContext_t* mqtt_context);

    inline bool is_client_session_present(void) const { return client_session_present; }
    inline bool is_broker_session_present(void) const { return broker_session_present; }
    
    inline NetworkContext_t* get_network_context(void) { return &network_context; }

private:
    esp_err_t connect_mqtt_socket(MQTTContext_t* mqtt_context);
    esp_err_t disconnect_mqtt_socket(MQTTContext_t* mqtt_context);

private:
    NetworkContext_t network_context;
    ThingConfig* thing_config;
    bool client_session_present = false;
    bool broker_session_present = false;
    StaticSemaphore_t xTlsContextSemaphoreBuffer;
};