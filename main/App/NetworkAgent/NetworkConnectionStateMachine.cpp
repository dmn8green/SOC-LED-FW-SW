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
    this->state = e_nc_state_unknown;
    this->state_handler = &NetworkConnectionStateMachine::off_state;
    return ret;
}


//*****************************************************************************
void NetworkConnectionStateMachine::handle_event(net_conn_agent_event_t event) {
    typedef NetworkConnectionStateMachine SM;

    net_conn_agent_state_t next_state = std::invoke(state_handler, this, event);
    if (next_state != this->state) {
        switch(next_state) {
            case e_nc_state_off                 : this->state_handler = &SM::off_state; break;
            case e_nc_state_connecting          : this->state_handler = &SM::connecting_state; break;
            case e_nc_state_connected           : this->state_handler = &SM::connected_state; break;
            case e_nc_state_testing_connection  : this->state_handler = &SM::testing_connection_state; break;
            case e_nc_state_no_connection_error : this->state_handler = &SM::no_connection_error_state; break;
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
net_conn_agent_state_t NetworkConnectionStateMachine::off_state(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "off state");
    net_conn_agent_state_t next_state = e_nc_state_off;

    // enter state
    if (this->state != e_nc_state_off) {
        ESP_LOGI(TAG, "Enter state off");
        this->state = e_nc_state_off;
    }

    switch (event) {
        case e_nc_event_net_connect:
            ESP_LOGI(TAG, "Connecting network");
            this->connection->connect();
            next_state = e_nc_state_connecting; 
            break;
        default:
            ESP_LOGI(TAG, "Invalid event %d while in state %d", event, this->state);
            break;
    }

    // leave state
    if (next_state != e_nc_state_off) {
        ESP_LOGI(TAG, "Leaving state off");
    }

    return next_state;
}

//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::connecting_state(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connecting handle event");
    net_conn_agent_state_t next_state = e_nc_state_connecting;

    // enter state
    if (this->state != e_nc_state_connecting) {
        ESP_LOGI(TAG, "Enter state connecting");
        this->state = e_nc_state_connecting;
    }

    switch (event) {
        case e_nc_event_net_connected     : next_state = e_nc_state_connected; break;
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
net_conn_agent_state_t NetworkConnectionStateMachine::connected_state(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connected handle event");
    net_conn_agent_state_t next_state = e_nc_state_connected;

    // enter state
    if (this->state != e_nc_state_connected) {
        ESP_LOGI(TAG, "Enter state connected");
        this->state = e_nc_state_connected;
    }

    switch (event) {
        case e_nc_event_net_disconnect:
            this->connection->disconnect();
            next_state = e_nc_state_disconnecting;
            break;

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
net_conn_agent_state_t NetworkConnectionStateMachine::disconnecting_state(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connecting handle event");
    net_conn_agent_state_t next_state = e_nc_state_connecting;

    // enter state
    if (this->state != e_nc_state_connecting) {
        ESP_LOGI(TAG, "Enter state connecting");
        this->state = e_nc_state_connecting;
    }

    switch (event) {
        case e_nc_event_net_disconnected  : next_state = e_nc_state_off; break;
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
net_conn_agent_state_t NetworkConnectionStateMachine::testing_connection_state(net_conn_agent_event_t event) {
    ESP_LOGI(TAG, "Connected handle event");
    net_conn_agent_state_t next_state = e_nc_state_testing_connection;

    // enter state
    if (this->state != e_nc_state_testing_connection) {
        ESP_LOGI(TAG, "Enter state connected");
        this->state = e_nc_state_testing_connection;
    }

    switch (event) {
        case e_nc_event_net_disconnected : next_state = e_nc_state_no_connection_error; break;
        case e_nc_event_timeout:
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            break;
        default:
            break;
    }

    // leave state
    if (next_state != e_nc_state_testing_connection) {
        ESP_LOGI(TAG, "Leaving state connected");
    }

    return next_state;
}


//*****************************************************************************
net_conn_agent_state_t NetworkConnectionStateMachine::no_connection_error_state(net_conn_agent_event_t event) {
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
