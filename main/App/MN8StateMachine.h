

#pragma once

#include "App/MN8Context.h"

#include "Utils/FreeRTOSTask.h"
#include "Utils/NoCopy.h"

#include "esp_err.h"

// connected means the network is connected and mqtt is also connected.
// if we loose mqtt connection, we go in re-establishing mqtt connection state.
//  that state will try turning on/off the network interfaces. and wait for both network and mqtt to be connected
//  if it times out then we go in connection error and show the error leds.
typedef enum {
    e_mn8_off,                        //?! The device hasn't been provisioned with AWS and with the MN8 cloud.
    e_mn8_connecting,                 //?! At least one interface is connecting to the cloud.
    e_mn8_connected,                  //?! At least one interface is connected to the cloud and the mqtt is also connected.
    e_mn8_connection_error,           //?! The interfaces are up but there is no connection.
    e_mn8_disconnecting,              //?! To indicate that both network connections are being disconnected manually.
    e_mn8_disconnected,               //?! The device has been provisioned but is not connected to the cloud. Both interface are down or unconfigured.
    e_mn8_unknown_state,
} mn8_state_t;

typedef enum {
    e_mn8_event_turn_off,
    e_mn8_event_turn_on,
    e_mn8_event_net_connect,
    e_mn8_event_net_connected,
    e_mn8_event_net_disconnect,
    e_mn8_event_net_disconnected,
    e_mn8_event_mqtt_connect,
    e_mn8_event_mqtt_connected,
    e_mn8_event_mqtt_disconnect,
    e_mn8_event_mqtt_disconnected,
    e_mn8_event_lost_network_connection,
    e_mn8_event_lost_mqtt_connection,
} mn8_event_t;

class MN8StateMachine : public NoCopy {
public:
    MN8StateMachine(void) = default;
    ~MN8StateMachine(void) = default;

public:
    esp_err_t setup(MN8Context* context);
    esp_err_t turn_on(void);
    esp_err_t turn_off(void);
    void handle_event(mn8_event_t event);

protected:
    mn8_state_t off_state              (mn8_event_t event);
    mn8_state_t disconnected_state     (mn8_event_t event);
    mn8_state_t disconnecting_state    (mn8_event_t event);
    mn8_state_t connecting_state       (mn8_event_t event);
    mn8_state_t connected_state        (mn8_event_t event);
    mn8_state_t connection_error_state (mn8_event_t event);

private:
    MN8Context* context = nullptr;

    mn8_state_t state = mn8_state_t::e_mn8_off;
    typedef mn8_state_t (MN8StateMachine::*StateFn)(mn8_event_t event);
    StateFn state_handler;
};