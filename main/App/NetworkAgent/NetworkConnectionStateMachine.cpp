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
    this->state_handler = &NetworkConnectionStateMachine::disabled_handle_event;
    return ret;
}


//*****************************************************************************
void NetworkConnectionStateMachine::handle_event(net_conn_agent_event_t event) {
    typedef NetworkConnectionStateMachine SM;

    net_conn_agent_state_t next_state = std::invoke(state_handler, this, event);
    if (next_state != this->state) {
        switch(next_state) {
            case e_nc_state_disabled            : this->state_handler = &SM::disabled_handle_event; break;
            case e_nc_state_connecting          : this->state_handler = &SM::connecting_handle_event; break;
            case e_nc_state_connected           : this->state_handler = &SM::connected_handle_event; break;
            case e_nc_state_disconnecting       : this->state_handler = &SM::disconnecting_handle_event; break;
            case e_nc_state_no_connection_error : this->state_handler = &SM::no_connection_error_handle_event; break;
            default:
                break;
        }

        ESP_LOGI(TAG, "State changed to %d", next_state);

        // Invoke the new state handler so that the enter state code is executed.
        // We do not support transient state, therefore the returned state should
        // be the same as the next state.
        std::invoke(state_handler, this, event);
    }
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::disabled_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Disabled handle event");
    net_conn_agent_state_t next_state = e_nc_state_disabled;

    // enter state
    if (this->state != e_nc_state_disabled) {
        ESP_LOGI(TAG, "Enter state disabled");
        this->state = e_nc_state_disabled;
    }

    switch (event) {
        case e_nc_event_net_connecting    : next_state = e_nc_state_connecting; break;
        case e_nc_event_net_connected     : next_state = e_nc_state_connected; break;
        default:
            ESP_LOGI(TAG, "Invalid event %d while in state %d", event, this->state);
            break;
    }

    // leave state
    if (next_state != e_nc_state_disabled) {
        ESP_LOGI(TAG, "Leaving state disabled");
    }

    return next_state;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::connecting_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connecting handle event");
    net_conn_agent_state_t next_state = e_nc_state_connecting;

    // enter state
    if (this->state != e_nc_state_connecting) {
        ESP_LOGI(TAG, "Enter state connecting");
        this->state = e_nc_state_connecting;
    }

    switch (event) {
        case e_nc_event_net_connected     : next_state = e_nc_state_connected; break;
        case e_nc_event_net_disconnecting : next_state = e_nc_state_disconnecting; break;
        case e_nc_event_net_disconnected  : next_state = e_nc_state_no_connection_error; break;
        case e_nc_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }

    // leave state
    if (next_state != e_nc_state_connecting) {
        ESP_LOGI(TAG, "Leaving state connecting");
    }

    return next_state;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::connected_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connected handle event");
    net_conn_agent_state_t next_state = e_nc_state_connected;

    // enter state
    if (this->state != e_nc_state_connected) {
        ESP_LOGI(TAG, "Enter state connected");
        this->state = e_nc_state_connected;
    }

    switch (event) {
        case e_nc_event_net_disconnecting: next_state = e_nc_state_disconnecting; break;
        case e_nc_event_net_disconnected : next_state = e_nc_state_no_connection_error; break;
        case e_nc_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }

    // leave state
    if (next_state != e_nc_state_connected) {
        ESP_LOGI(TAG, "Leaving state connected");
    }

    return next_state;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::disconnecting_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Disconnecting handle event");
    net_conn_agent_state_t next_state = e_nc_state_disconnecting;

    // enter state
    if (this->state != e_nc_state_disconnecting) {
        ESP_LOGI(TAG, "Enter state disconnecting");
        this->state = e_nc_state_disconnecting;
    }

    switch (event) {
        case e_nc_event_net_connecting  : next_state = e_nc_state_connecting; break;
        case e_nc_event_net_connected   : next_state = e_nc_state_connected; break;
        case e_nc_event_net_disconnected: next_state = e_nc_state_no_connection_error; break;
        case e_nc_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }

    // leave state
    if (next_state != e_nc_state_disconnecting) {
        ESP_LOGI(TAG, "Leaving state disconnecting");
    }

    return next_state;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::no_connection_error_handle_event(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "No connection error handle event");
    net_conn_agent_state_t next_state = e_nc_state_no_connection_error;

    // enter state
    if (this->state != e_nc_state_no_connection_error) {
        ESP_LOGI(TAG, "Enter state connection error");
        this->state = e_nc_state_no_connection_error;
    }

    switch (event) {
        case e_nc_event_net_connecting:
            next_state = e_nc_state_connecting;
            break;
        case e_nc_event_net_connected:
            next_state = e_nc_state_connected;
            break;
        case e_nc_event_net_disconnecting:
            ESP_LOGI(TAG, "Already disconnected and in error???");
            break;
        case e_nc_event_net_disconnected:
            ESP_LOGI(TAG, "Already disconnected and in error???");
            break;
        case e_nc_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, Reboot?");
            break;
        default:
            break;
    }

    // leave state
    if (next_state != e_nc_state_no_connection_error) {
        ESP_LOGI(TAG, "Leaving state connection error");
    }

    return next_state;
}
