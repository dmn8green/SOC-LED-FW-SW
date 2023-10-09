//******************************************************************************
/**
 * @file NetworkConnectionStateMachine.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief NetworkConnectionStateMachine class definition
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************
#pragma once

#include "App/NetworkAgent/NetworkConnection.h"
#include "esp_err.h"

typedef enum {
    e_network_connection_state_disabled,              //?! To indicate that the network connection are all disabled
    e_network_connection_state_connecting,            //?! To indicate that at least one network connection is connecting
    e_network_connection_state_connected,             //?! To indicate that at least one network connection is connected
    e_network_connection_state_testing_connection,    //?! To indicate that at least one network connection is testing the connection
    e_network_connection_state_disconnecting,         //?! To indicate that both network connections are disconnecting
    e_network_connection_state_no_connection_error    //?! To indicate that both network connections are disconnected, network error
} net_conn_agent_state_t;

typedef enum {
    e_network_connection_event_net_connecting,
    e_network_connection_event_net_connected,
    e_network_connection_event_net_disconnecting,
    e_network_connection_event_net_disconnected,
    e_network_connection_event_timeout,
    e_network_connection_event_wifi_down,
    e_network_connection_event_eth_down
} net_conn_agent_event_t;

//******************************************************************************
/**
 * @brief NetworkConnectionStateMachine class
 * 
 * This class is responsible for managing the network connection state machine.
 */
class NetworkConnectionStateMachine {
public:
    NetworkConnectionStateMachine(void) = default;
    virtual ~NetworkConnectionStateMachine(void) = default;

public:
    esp_err_t setup(NetworkConnection* connection);
    void handle_event(net_conn_agent_event_t event);

protected:
    net_conn_agent_state_t disabled_handle_event(net_conn_agent_event_t event);
    net_conn_agent_state_t connected_handle_event(net_conn_agent_event_t event);
    net_conn_agent_state_t connecting_handle_event(net_conn_agent_event_t event);
    net_conn_agent_state_t disconnecting_handle_event(net_conn_agent_event_t event);
    net_conn_agent_state_t no_connection_error_handle_event(net_conn_agent_event_t event);

private:
    NetworkConnection* connection = nullptr;
    net_conn_agent_state_t state = net_conn_agent_state_t::e_network_connection_state_disabled;
    typedef net_conn_agent_state_t (NetworkConnectionStateMachine::*StateFn)(net_conn_agent_event_t event);
    StateFn state_handler;
};