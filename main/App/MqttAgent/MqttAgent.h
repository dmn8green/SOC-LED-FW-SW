//******************************************************************************
/**
 * @file MQTTAgent.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief MQTTAgent class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "Utils/FreeRTOSTask.h"
#include "App/MqttAgent/MqttContext.h"
#include "App/MqttAgent/MqttConnection.h"
#include "App/Configuration/ThingConfig.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_eth_com.h"

#include <freertos/queue.h>
#include <freertos/event_groups.h>

#define MQTT_AGENT_TASK_STACK_SIZE 5000
#define MQTT_AGENT_TASK_PRIORITY 5
#define MQTT_AGENT_TASK_CORE_NUM 0
#define MQTT_AGENT_TASK_NAME "mqtt_agent"

typedef void (*mqttCallbackFn)(char *, unsigned int, uint8_t *, unsigned int);

//******************************************************************************
/**
 * @brief MQTTAgent class
 * 
 * This class is responsible for managing the MQTT connection.
 */
class MqttAgent : public FreeRTOSTask {
public:  
    MqttAgent(void) : FreeRTOSTask(
        MQTT_AGENT_TASK_STACK_SIZE,
        MQTT_AGENT_TASK_PRIORITY,
        MQTT_AGENT_TASK_CORE_NUM
    ) {};
    ~MqttAgent(void) = default;

    esp_err_t setup(ThingConfig* thing_config);

public:
    esp_err_t subscribe(const char *topic, mqttCallbackFn callback, void* context);
    esp_err_t unsubscribe(const char *topic);
    esp_err_t publish_message(const char *topic, const char *payload, uint8_t retry_count = 0);

protected:
    virtual void taskFunction(void) override;
    virtual const char* task_name(void) override { return MQTT_AGENT_TASK_NAME; }

    void onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onWifiEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void on_mqtt_pubsub_event(
        struct MQTTContext * context,
        struct MQTTPacketInfo * packet_info,
        struct MQTTDeserializedInfo * deserialized_info);

    static void sOnEthEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((MqttAgent*)arg)->onEthEvent(event_base, event_id, event_data); }
    static void sOnWifiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((MqttAgent*)arg)->onWifiEvent(event_base, event_id, event_data); }
    static void sOnIPEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((MqttAgent*)arg)->onIPEvent(event_base, event_id, event_data); }

    static void sOn_mqtt_pubsub_event(
        struct MQTTContext * pContext,
        struct MQTTPacketInfo * pPacketInfo,
        struct MQTTDeserializedInfo * pDeserializedInfo,
        void* context
    ) {
        ((MqttAgent*)context)->on_mqtt_pubsub_event(pContext, pPacketInfo, pDeserializedInfo);
    }

private:
    esp_err_t process_mqtt_loop(void);

private:

    ThingConfig* thing_config;
    QueueHandle_t queue;
    EventGroupHandle_t event_group;

    MqttConnection mqtt_connection;
    MqttContext mqtt_context;
};