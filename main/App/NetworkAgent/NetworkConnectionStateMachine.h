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
    e_nc_state_off,                   //?! To indicate that the network connection are all disabled
    e_nc_state_connecting,            //?! To indicate that at least one network connection is connecting
    e_nc_state_connected,             //?! To indicate that at least one network connection is connected
    e_nc_state_disconnecting,         //?! To indicate that at least one network connection is disconnecting
    e_nc_state_testing_connection,    //?! To indicate that at least one network connection is testing the connection
    e_nc_state_no_connection_error,   //?! To indicate that both network connections are disconnected, network error
    e_nc_state_restarting,
    e_nc_state_unknown
} net_conn_agent_state_t;

typedef enum {
    e_nc_event_net_connect,
    e_nc_event_net_connecting,
    e_nc_event_net_connected,
    e_nc_event_net_disconnect,
    e_nc_event_net_disconnected,
    e_nc_event_timeout,
    e_nc_event_wifi_down,
    e_nc_event_eth_down,
    e_nc_event_restart
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
    net_conn_agent_state_t off_state(net_conn_agent_event_t event);
    net_conn_agent_state_t connecting_state(net_conn_agent_event_t event);
    net_conn_agent_state_t connected_state(net_conn_agent_event_t event);
    net_conn_agent_state_t disconnecting_state(net_conn_agent_event_t event);
    net_conn_agent_state_t testing_connection_state(net_conn_agent_event_t event);
    net_conn_agent_state_t no_connection_error_state(net_conn_agent_event_t event);
    net_conn_agent_state_t restarting_state(net_conn_agent_event_t event);

private:
    NetworkConnection* connection = nullptr;
    net_conn_agent_state_t state = net_conn_agent_state_t::e_nc_state_off;
    typedef net_conn_agent_state_t (NetworkConnectionStateMachine::*StateFn)(net_conn_agent_event_t event);
    StateFn state_handler;
};