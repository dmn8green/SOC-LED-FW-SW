#include "MN8StateMachine.h"

#include "mdns_broadcaster.h"
#include "udp_server.h"
#include "web_server.h"

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
esp_err_t MN8StateMachine::setup(MN8Context *context)
{
    esp_err_t ret = ESP_OK;
    this->context = context;

    this->state = e_mn8_unknown_state;
    this->state_handler = &MN8StateMachine::off_state;

    if (!this->context->get_thing_config().is_configured())
    {
        this->state = e_mn8_iot_unprovisioned;
        this->state_handler = &MN8StateMachine::iot_unprovisioned_state;
    }

    initialise_mdns();
    start_udp_server(this->context);
    start_webserver();

    return ret;
}

//*****************************************************************************
esp_err_t MN8StateMachine::turn_on(void)
{
    this->handle_event(e_mn8_event_turn_on);
    return ESP_OK;
}

//*****************************************************************************
esp_err_t MN8StateMachine::turn_off(void)
{
    this->handle_event(e_mn8_event_turn_off);
    return ESP_OK;
}

//*****************************************************************************
const char *MN8StateMachine::get_current_state_name(void)
{
    switch (this->state)
    {
    case e_mn8_off:
        return "off";
    case e_mn8_connecting:
        return "connecting";
    case e_mn8_connected:
        return "connected";
    case e_mn8_iot_unprovisioned:
        return "iot_unprovisioned";
    case e_mn8_cp_unprovisioned:
        return "cp_unprovisioned";
    case e_mn8_connection_error:
        return "connection_error";
    case e_mn8_disconnecting:
        return "disconnecting";
    case e_mn8_disconnected:
        return "disconnected";
    default:
        return "unknown";
    }
}

//*****************************************************************************
void MN8StateMachine::handle_event(mn8_event_t event)
{
    typedef MN8StateMachine SM;

    ESP_LOGI(TAG, "Handling event %d while in state %d", event, this->state);
    mn8_state_t next_state = std::invoke(state_handler, this, event);
    if (next_state != this->state)
    {
        switch (next_state)
        {
        case e_mn8_off:
            this->state_handler = &SM::off_state;
            break;
        case e_mn8_cp_unprovisioned:
            this->state_handler = &SM::cp_unprovisioned_state;
            break;
        case e_mn8_connecting:
            this->state_handler = &SM::connecting_state;
            break;
        case e_mn8_connected:
            this->state_handler = &SM::connected_state;
            break;
        case e_mn8_connection_error:
            this->state_handler = &SM::connection_error_state;
            break;
        case e_mn8_disconnecting:
            this->state_handler = &SM::disconnecting_state;
            break;
        case e_mn8_disconnected:
            this->state_handler = &SM::disconnected_state;
            break;
        default:
            break;
        }

        ESP_LOGI(TAG, "State changed to %d", next_state);

        // Invoke the new state handler so that the enter state code is executed.
        // We do not support transient state, therefore the returned state should
        // be the same as the next state.
        std::invoke(state_handler, this, event);
    }
    ESP_LOGI(TAG, "Handled event %d now in state %d", event, this->state);
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
mn8_state_t MN8StateMachine::off_state(mn8_event_t event)
{
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_off)
    {
        ESP_LOGI(TAG, "Entering off state");
        this->state = e_mn8_off;
    }

    switch (event)
    {
    case e_mn8_event_turn_on:
        this->context->get_network_connection_agent().connect();
        next_state = e_mn8_connecting;
        break;
    default:
        next_state = e_mn8_off;
    }

    if (next_state != e_mn8_off)
    {
        ESP_LOGI(TAG, "Leaving off state");
    }

    return next_state;
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
mn8_state_t MN8StateMachine::iot_unprovisioned_state(mn8_event_t event)
{
    // This is an end sstate a reboot is required.
    this->context->get_led_task_0().set_state("iot_unprovisioned", 0);
    this->context->get_led_task_1().set_state("iot_unprovisioned", 0);

    switch (event)
    {
    case e_mn8_event_turn_on:
        this->context->get_network_connection_agent().connect();
        break;
    default:
        break;
    }

    return e_mn8_iot_unprovisioned;
}

//*****************************************************************************
// stuck here until configured and rebooted
mn8_state_t MN8StateMachine::cp_unprovisioned_state(mn8_event_t event)
{
    // This is an end sstate a reboot is required.
    this->context->get_led_task_0().set_state("cp_unprovisioned", 0);
    this->context->get_led_task_1().set_state("cp_unprovisioned", 0);

    switch (event)
    {
    default:
        break;
    }

    return e_mn8_cp_unprovisioned;
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
mn8_state_t MN8StateMachine::connecting_state(mn8_event_t event)
{
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_connecting)
    {
        ESP_LOGI(TAG, "Entering connecting state");
        this->context->get_network_connection_agent().restart();
        this->state = e_mn8_connecting;
    }

    switch (event)
    {
    // case e_mn8_event_net_connected:
    case e_mn8_event_mqtt_connected:
        if (this->context->get_network_connection_agent().is_connected() &&
            this->context->get_mqtt_agent().is_connected())
        {
            if (!this->context->get_charge_point_config().is_configured())
            {
                // chargepoint is not configure.  We will not be able to do anything
                // useful.  Go to chargepoint unprovisioned state.
                ESP_LOGI(TAG, "ChargePoint is not provisioned");
                next_state = e_mn8_cp_unprovisioned;
            }
            else
            {
                next_state = e_mn8_connected;
            }
        }

        break;

    default:
        next_state = e_mn8_connecting;
    }

    if (next_state != e_mn8_connecting)
    {
        ESP_LOGI(TAG, "Leaving connecting state %d", next_state);
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
mn8_state_t MN8StateMachine::connected_state(mn8_event_t event)
{
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_connected)
    {
        ESP_LOGI(TAG, "Entering connected state");
        this->state = e_mn8_connected;
        this->context->get_iot_heartbeat().send_heartbeat();

        // turn off breathing pattern, we will get a new one from the broker.
        this->context->get_led_task_0().set_state("waiting_4_first_state", 0);
        this->context->get_led_task_1().set_state("waiting_4_first_state", 0);
    }

    switch (event)
    {
    case e_mn8_event_mqtt_disconnected:
    case e_mn8_event_lost_mqtt_connection:
    case e_mn8_event_net_disconnected:
    case e_mn8_event_lost_network_connection:
        // Maybe something is wrong with the network/wifi.
        // Go back to connecting, that will trigger a disconnect and
        // reconnect.
        // TEMP HACK TO USE WHILE TESTING
        ESP_LOGI(TAG, "Lost connection, going back to connecting");
        this->context->get_led_task_0().set_state("no_connection", 0);
        this->context->get_led_task_1().set_state("no_connection", 0);

        next_state = e_mn8_connecting;
        break;
    default:
        next_state = e_mn8_connected;
    }

    if (next_state != e_mn8_connected)
    {
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
mn8_state_t MN8StateMachine::connection_error_state(mn8_event_t event)
{
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_connection_error)
    {
        ESP_LOGI(TAG, "Entering connection_error state");
        this->context->get_led_task_0().set_state("offline", 0);
        this->context->get_led_task_1().set_state("offline", 0);

        this->state = e_mn8_connection_error;
    }

    switch (event)
    {
    default:
        next_state = e_mn8_connection_error;
    }

    if (next_state != e_mn8_unknown_state)
    {
        ESP_LOGI(TAG, "Leaving connection_error state");
    }

    return next_state;
}

//*****************************************************************************
mn8_state_t MN8StateMachine::disconnecting_state(mn8_event_t event)
{
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_disconnecting)
    {
        ESP_LOGI(TAG, "Entering disconnecting state");
        this->state = e_mn8_disconnecting;
    }

    switch (event)
    {
    case e_mn8_event_mqtt_disconnected:
        next_state = e_mn8_disconnected;
        break;
    default:
        next_state = e_mn8_disconnecting;
    }

    if (next_state != e_mn8_unknown_state)
    {
        ESP_LOGI(TAG, "Leaving disconnecting state");
    }

    return next_state;
}

//*****************************************************************************
mn8_state_t MN8StateMachine::disconnected_state(mn8_event_t event)
{
    mn8_state_t next_state = e_mn8_unknown_state;

    if (this->state != e_mn8_disconnected)
    {
        ESP_LOGI(TAG, "Entering disconnected state");
        this->state = e_mn8_disconnected;
    }

    switch (event)
    {
    case e_mn8_event_net_connected:
        next_state = e_mn8_connected;
        break;
    default:
        next_state = e_mn8_disconnected;
    }

    if (next_state != e_mn8_unknown_state)
    {
        ESP_LOGI(TAG, "Leaving disconnected state");
    }

    return next_state;
}
