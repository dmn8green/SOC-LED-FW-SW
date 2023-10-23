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

public:
    esp_err_t setup(ThingConfig* thing_config);
    void connect(void);
    void disconnect(void);
    inline bool is_connected(void) { return this->connected; }

    esp_err_t subscribe(const char *topic, mqttCallbackFn callback, void* context);
    esp_err_t unsubscribe(const char *topic);
    esp_err_t publish_message(const char *topic, const char *payload, uint8_t retry_count = 0);

    typedef enum {
        e_mqtt_agent_connected,
        e_mqtt_agent_disconnected,
    } event_t;

    typedef void(*event_callback_t)(event_t event, void* context);
    inline void register_event_callback(event_callback_t callback, void* context) {
        this->event_callback = callback;
        this->event_callback_context = context;
    }

    typedef void (*handle_incoming_mqtt_fn)(
        const char* pTopicName,
        uint16_t topicNameLength,
        const char* pPayload,
        size_t payloadLength,
        uint16_t packetIdentifier,
        void* context
    );
    void register_handle_incoming_mqtt(handle_incoming_mqtt_fn callback, void* context);

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
        ESP_LOGI("MqttAgent", "sOn_mqtt_pubsub_event agent is %p mn8 %p", context, ((MqttAgent*)context)->handle_incoming_mqtt_context);
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

    event_callback_t event_callback;
    void* event_callback_context;

    handle_incoming_mqtt_fn handle_incoming_mqtt;
    void* handle_incoming_mqtt_context;

    bool connected = false;
    SemaphoreHandle_t mqtt_mutex;
};