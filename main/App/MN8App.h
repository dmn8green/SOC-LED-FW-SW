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

#include "App/MN8Context.h"
#include "App/MN8StateMachine.h"

#include "Utils/Singleton.h"
#include "Utils/NoCopy.h"

#include "freertos/queue.h"

/**
 * @brief MN8App class.
 * 
 * This is the main class of the project.
 * It is a singleton class.
 * It holds the overall system logic along with the interfaces to the different
 * peripherals such as the (wifi/ethernet) connection, the led strip, etc.
 * 
 * It is really a singleton so that the repl interface don't have to
 * carry the app instance around.
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

    inline MN8Context & get_context(void) { return context; }

    // To be backward compatible for now.
    inline bool is_iot_thing_provisioned(void) { return context.is_iot_thing_provisioned(); } 
    inline NetworkConnectionAgent& get_network_connection_agent(void) { return this->context.get_network_connection_agent(); }
    inline MqttAgent& get_mqtt_agent(void) { return this->context.get_mqtt_agent(); }
    inline LedTaskSpi& get_led_task_0(void) { return this->context.get_led_task_0(); }
    inline LedTaskSpi& get_led_task_1(void) { return this->context.get_led_task_1(); }

protected:
    void on_network_event(NetworkConnectionAgent::event_t event);
    void on_mqtt_event(MqttAgent::event_t event);

    static void sOn_network_event(NetworkConnectionAgent::event_t event, void* context) { ((MN8App*)context)->on_network_event(event); }
    static void sOn_mqtt_event(MqttAgent::event_t event, void* context) { ((MN8App*)context)->on_mqtt_event(event); }

    void on_incoming_mqtt( 
        const char* pTopicName,
        uint16_t topicNameLength,
        const char* pPayload,
        size_t payloadLength,
        uint16_t packetIdentifier
    );

    static void sOn_incoming_mqtt( 
        const char* pTopicName,
        uint16_t topicNameLength,
        const char* pPayload,
        size_t payloadLength,
        uint16_t packetIdentifier,
        void* context
    ) {
        ((MN8App*)context)->on_incoming_mqtt(pTopicName, topicNameLength, pPayload, payloadLength, packetIdentifier);
    }


private:
    esp_err_t setup_and_start_led_tasks(void);

private:
    MN8Context context;
    MN8StateMachine state_machine;
    QueueHandle_t message_queue;
};  // class MN8App