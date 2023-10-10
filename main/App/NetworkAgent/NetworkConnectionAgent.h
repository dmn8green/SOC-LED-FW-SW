//******************************************************************************
/**
 * @file NetworkConnectionAgent.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief NetworkConnectionAgent class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "Utils/FreeRTOSTask.h"
#include "App/NetworkAgent/NetworkConnection.h"
#include "App/NetworkAgent/NetworkConnectionStateMachine.h"

#include "freertos/queue.h"

#define CONNECTION_STATE_MACHINE_TASK_STACK_SIZE 5000
#define CONNECTION_STATE_MACHINE_TASK_PRIORITY 5
#define CONNECTION_STATE_MACHINE_TASK_CORE_NUM 0
#define CONNECTION_STATE_MACHINE_TASK_NAME "net_agent"

//******************************************************************************
/**
 * @brief NetworkConnectionAgent class
 * 
 * This class is responsible for managing the network connection state machine.
 * It is responsible for setting up the network interfaces and connections.
 * It is also responsible for handling network events.
 */
class NetworkConnectionAgent : public FreeRTOSTask {
public:
    NetworkConnectionAgent(void) : 
        FreeRTOSTask(
            CONNECTION_STATE_MACHINE_TASK_STACK_SIZE,
            CONNECTION_STATE_MACHINE_TASK_PRIORITY,
            CONNECTION_STATE_MACHINE_TASK_CORE_NUM
        ) {};
    virtual ~NetworkConnectionAgent(void) = default;

public:
    esp_err_t setup(void);

    // Accessors
    inline NetworkInterface*   get_wifi_interface(void) { return this->connection.get_wifi_interface(); }
    inline WifiConnection*     get_wifi_connection(void) { return this->connection.get_wifi_connection(); }

    inline NetworkInterface*   get_ethernet_interface(void) { return this->connection.get_ethernet_interface(); }
    inline EthernetConnection* get_ethernet_connection(void) { return this->connection.get_ethernet_connection(); }

    inline Connection* get_connection(const char* interface) { return this->connection.get_connection(interface); };

protected:
    virtual void taskFunction(void) override;
    virtual const char* task_name(void) override { return CONNECTION_STATE_MACHINE_TASK_NAME; }

    void onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onWifiEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
    void onIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static void sOnEthEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((NetworkConnectionAgent*)arg)->onEthEvent(event_base, event_id, event_data); }
    static void sOnWifiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((NetworkConnectionAgent*)arg)->onWifiEvent(event_base, event_id, event_data); }
    static void sOnIPEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((NetworkConnectionAgent*)arg)->onIPEvent(event_base, event_id, event_data); }

private:
    QueueHandle_t update_queue;
    NetworkConnection connection;
    NetworkConnectionStateMachine state_machine;
};