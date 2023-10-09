//******************************************************************************
/**
 * @file NetworkConnectionStateMachine.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief NetworkConnectionStateMachine class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#include "NetworkConnectionStateMachine.h"

#include "esp_log.h"
#include "esp_check.h"
#include "pin_def.h"

#include <functional>

static const char *TAG = "NetworkConnectionStateMachine";

//*****************************************************************************
/**
 * @brief Setup the network connection agent.
 * 
 * This function will setup the network connection agent.  It will create the
 * wifi and ethernet interfaces and connections.  It will also initialize the
 * wifi and ethernet connections.
 * 
 * @return esp_err_t 
 */
esp_err_t NetworkConnectionStateMachine::setup(NetworkConnection* connection)
{
    esp_err_t ret = ESP_OK;
    this->connection = connection;
    this->state = e_network_connection_state_disabled;
    this->state_handler = &NetworkConnectionStateMachine::disabled_handle_event;
    return ret;
}


//*****************************************************************************
void NetworkConnectionStateMachine::handle_event(net_conn_agent_event_t event) {
    // net_conn_agent_state_t next_state = this->state_handler(event);
    net_conn_agent_state_t next_state = std::invoke(state_handler, this, event);
    if (next_state != this->state) {
        // call leave method,
        this->state = next_state;
        // call enter method
        ESP_LOGI(TAG, "State changed to %d", this->state);
    }
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::disabled_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Disabled handle event");
    switch (event) {
        case e_network_connection_event_net_connecting:
            this->state_handler = &NetworkConnectionStateMachine::connecting_handle_event;
            return e_network_connection_state_connecting;
            break;
        case e_network_connection_event_net_connected:
            this->state_handler = &NetworkConnectionStateMachine::connected_handle_event;
            return e_network_connection_state_connected;
            break;
        case e_network_connection_event_net_disconnecting:
            ESP_LOGE(TAG, "Cannot disconnect, network is disabled");
            break;
        case e_network_connection_event_net_disconnected:
            ESP_LOGE(TAG, "Cannot disconnect, network is disabled");
            break;
        case e_network_connection_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }
    this->state_handler = &NetworkConnectionStateMachine::disabled_handle_event;
    return e_network_connection_state_disabled;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::connecting_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connecting handle event");
    switch (event) {
        case e_network_connection_event_net_connecting:
            ESP_LOGI(TAG, "Already connecting");
            break;
        case e_network_connection_event_net_connected:
            this->state_handler = &NetworkConnectionStateMachine::connected_handle_event;
            return e_network_connection_state_connected;
            break;
        case e_network_connection_event_net_disconnecting:
            this->state_handler = &NetworkConnectionStateMachine::disconnecting_handle_event;
            return e_network_connection_state_disconnecting;
            break;
        case e_network_connection_event_net_disconnected:
            this->state_handler = &NetworkConnectionStateMachine::no_connection_error_handle_event;
            return e_network_connection_state_no_connection_error;
            break;
        case e_network_connection_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }
    this->state_handler = &NetworkConnectionStateMachine::connecting_handle_event;
    return e_network_connection_state_connecting;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::connected_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connected handle event");
    switch (event) {
        case e_network_connection_event_net_connecting:
            this->state_handler = &NetworkConnectionStateMachine::connecting_handle_event;
            return e_network_connection_state_connecting;
            break;
        case e_network_connection_event_net_connected:
            ESP_LOGI(TAG, "Already connected");
            break;
        case e_network_connection_event_net_disconnecting:
            this->state_handler = &NetworkConnectionStateMachine::disconnecting_handle_event;
            return e_network_connection_state_disconnecting;
            break;
        case e_network_connection_event_net_disconnected:
            this->state_handler = &NetworkConnectionStateMachine::no_connection_error_handle_event;
            return e_network_connection_state_no_connection_error;
            break;
        case e_network_connection_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }
    this->state_handler = &NetworkConnectionStateMachine::connected_handle_event;
    return e_network_connection_state_connected;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::disconnecting_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Disconnecting handle event");
    switch (event) {
        case e_network_connection_event_net_connecting:
            this->state_handler = &NetworkConnectionStateMachine::connecting_handle_event;
            return e_network_connection_state_connecting;
            break;
        case e_network_connection_event_net_connected:
            this->state_handler = &NetworkConnectionStateMachine::connected_handle_event;
            return e_network_connection_state_connected;
            break;
        case e_network_connection_event_net_disconnecting:
            ESP_LOGI(TAG, "Already disconnecting");
            break;
        case e_network_connection_event_net_disconnected:
            this->state_handler = &NetworkConnectionStateMachine::no_connection_error_handle_event;
            return e_network_connection_state_no_connection_error;
            break;
        case e_network_connection_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }
    this->state_handler = &NetworkConnectionStateMachine::disconnecting_handle_event;
    return e_network_connection_state_disconnecting;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::no_connection_error_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "No connection error handle event");
    switch (event) {
        case e_network_connection_event_net_connecting:
            this->state_handler = &NetworkConnectionStateMachine::connecting_handle_event;
            return e_network_connection_state_connecting;
            break;
        case e_network_connection_event_net_connected:
            this->state_handler = &NetworkConnectionStateMachine::connected_handle_event;
            return e_network_connection_state_connected;
            break;
        case e_network_connection_event_net_disconnecting:
            ESP_LOGI(TAG, "Already disconnected and in error???");
            break;
        case e_network_connection_event_net_disconnected:
            ESP_LOGI(TAG, "Already disconnected and in error???");
            break;
        case e_network_connection_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, Reboot?");
            break;
        default:
            break;
    }
    this->state_handler = &NetworkConnectionStateMachine::no_connection_error_handle_event;
    return e_network_connection_state_no_connection_error;
}
