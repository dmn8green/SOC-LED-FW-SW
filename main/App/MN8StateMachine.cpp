#include "MN8StateMachine.h"

#include "esp_log.h"
#include "esp_check.h"
#include "pin_def.h"

#include <functional>

static const char *TAG = "MN8StateMachine";

// unprovisioned (iot thing, chargepoint, wifi, net)
//   no transition, must reboot once provisioned
//
// when provisionned on boot
//   go to connecting state
//
// when in on connecting
//   -- on enter
//     -- start network
//   -- on event
//     -- net connected go to mqtt connecting state
//     -- on timeout
//        stop network
//        go to error state
// 
// when mqtt connecting
//   -- on event
//     -- mqtt connected go to connected state
//     -- timeout
//        if less than three tries
//          stop network  // force a connection reset.
//          go to connecting state
//        if mode than three tries
//          stop network  // force a connection reset.
//          go to connection error state
//
// when live/connected (the system is running)
//   -- on enter
//   -- on event
//     -- on lost network event go to connection error state
//     -- on lost mqtt connection
//        stop network  // force a connection reset.
//        go to connecting state
//
// when in disconnecting
//   -- on disconnected event
//      go to disconnected state
//
// when in disconnected state
//
// when in connection error state
//   -- on event
//      -- net connected and mqtt connected go to connected state

//*****************************************************************************
esp_err_t MN8StateMachine::setup(MN8Context* context)
{
    esp_err_t ret = ESP_OK;
    this->context = context;

    this->state = e_mn8_unknown_state;
    this->state_handler = &MN8StateMachine::off_state;

    return ret;
}

//*****************************************************************************
esp_err_t MN8StateMachine::turn_on(void) {
    bool is_provisioned = 
        context->is_iot_thing_provisioned() &&
        //!context->is_cp_provisioned() ||
        context->get_network_connection_agent().is_configured();

    // figure out the initial state.
    ESP_LOGI(TAG, "is_provisioned = %d", is_provisioned);
    if (is_provisioned) {
        this->handle_event(e_mn8_event_turn_on);
    }

    return ESP_OK;
}

//*****************************************************************************
esp_err_t MN8StateMachine::turn_off(void) {
    this->handle_event(e_mn8_event_turn_off);
    return ESP_OK;
}

//*****************************************************************************
void MN8StateMachine::handle_event(mn8_event_t event) {
    typedef MN8StateMachine SM;

    mn8_state_t next_state = std::invoke(state_handler, this, event);
    if (next_state != this->state) {
        switch(next_state) {
            case e_mn8_off              : this->state_handler = &SM::off_state; break;
            case e_mn8_connecting       : this->state_handler = &SM::connecting_state; break;
            case e_mn8_connected        : this->state_handler = &SM::connected_state; break;
            case e_mn8_connection_error : this->state_handler = &SM::connection_error_state; break;
            case e_mn8_disconnecting    : this->state_handler = &SM::disconnecting_state; break;
            case e_mn8_disconnected     : this->state_handler = &SM::disconnected_state; break;
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
/**
 * @brief Handle the unprovisioned state.
 * 
 * When unprovisioned, there are no possible transition.  Unit must be rebooted
 * to transition to the connect state.
 * 
 * @param event The event to handle.
 */
mn8_state_t MN8StateMachine::off_state(mn8_event_t event) {
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_off) {
        ESP_LOGI(TAG, "Entering off state");
        this->state = e_mn8_off;
    }

    switch (event) {
        case e_mn8_event_turn_on:
            this->context->get_network_connection_agent().connect();
            next_state = e_mn8_connecting;
            break;
        default:
            next_state = e_mn8_off;
    }

    if (next_state != e_mn8_off) {
        ESP_LOGI(TAG, "Leaving off state");
    }

    return next_state;
}

//*****************************************************************************
/**
 * @brief Handle the connecting state.
 * 
 * When connecting, we wait on the network agent and mqtt agent to report a
 * successful connection.
 * 
 * @param event The event to handle.
 */
mn8_state_t MN8StateMachine::connecting_state(mn8_event_t event) {
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_connecting) {
        ESP_LOGI(TAG, "Entering connecting state");
        this->state = e_mn8_connecting;
    }

    switch (event) {
        case e_mn8_event_net_connected:
        case e_mn8_event_mqtt_connected:
            // if (this->context->get_network_connection_agent().is_connected() &&
            //     this->context->get_mqtt_agent().is_connected())
            // {
            next_state = e_mn8_connected;
            
            break;
        default:
            next_state = e_mn8_connecting;
    }

    if (next_state != e_mn8_connecting) {
        ESP_LOGI(TAG, "Leaving connecting state");
    }

    return next_state;
}

//*****************************************************************************
/**
 * @brief Handle the connected state.
 * 
 * When connected, the system is live.  We will be receiving message from the
 * proxy broker.
 * 
 * We only care about loosing the network.
 * 
 * @param event The event to handle.
 */
mn8_state_t MN8StateMachine::connected_state(mn8_event_t event) {
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_connected) {
        ESP_LOGI(TAG, "Entering connected state");
        this->state = e_mn8_connected;
    }

    switch (event) {
        case e_mn8_event_lost_mqtt_connection:
            // Maybe something is wrong with the network/wifi. 
            // Force a reconnect?
            // this->context->get_network_agent().restart();
            next_state = e_mn8_connecting;
            break;
        case e_mn8_event_lost_network_connection:
            // This means the network gave up trying to reconnect.
            // Go to the error state, or maybe reboot after a while?
            next_state = e_mn8_connection_error;
            break;
        default:
            next_state = e_mn8_connected;
    }

    if (next_state != e_mn8_connected) {
        ESP_LOGI(TAG, "Leaving connected state");
    }

    return next_state;
}


//*****************************************************************************
/**
 * @brief Handle the connection error state.
 * 
 * When in the connection error state, we will try to reconnect the network
 * and/or reboot the device every so often?  Or we leave the logic back to the
 * network sm.
 * 
 * @param event The event to handle.
 */
mn8_state_t MN8StateMachine::connection_error_state(mn8_event_t event) {
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_connection_error) {
        ESP_LOGI(TAG, "Entering connection_error state");
        this->state = e_mn8_connection_error;
    }

    switch (event) {
        default:
            next_state = e_mn8_connection_error;
    }

    if (next_state != e_mn8_unknown_state) {
        ESP_LOGI(TAG, "Leaving connection_error state");
    }

    return next_state;
}

//*****************************************************************************
mn8_state_t MN8StateMachine::disconnecting_state(mn8_event_t event) {
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_disconnecting) {
        ESP_LOGI(TAG, "Entering disconnecting state");
        this->state = e_mn8_disconnecting;
    }

    switch (event) {
        default:
            next_state = e_mn8_disconnecting;
    }

    if (next_state != e_mn8_unknown_state) {
        ESP_LOGI(TAG, "Leaving disconnecting state");
    }

    return next_state;
}

//*****************************************************************************
mn8_state_t MN8StateMachine::disconnected_state(mn8_event_t event) {
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_disconnected) {
        ESP_LOGI(TAG, "Entering disconnected state");
        this->state = e_mn8_disconnected;
    }

    switch (event) {
        case e_mn8_event_net_connect:
            next_state = e_mn8_connecting;
            break;
        default:
            next_state = e_mn8_disconnected;
    }

    if (next_state != e_mn8_unknown_state) {
        ESP_LOGI(TAG, "Leaving disconnected state");
    }

    return next_state;
}



