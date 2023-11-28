//******************************************************************************
/**
 * @file NetworkConnectionAgent.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief NetworkConnectionAgent class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#include "NetworkConnectionAgent.h"
#include "Network/Utils/ethernet_init.h"

#include "esp_log.h"
#include "esp_check.h"
#include "pin_def.h"

#include <functional>

static const char *TAG = CONNECTION_STATE_MACHINE_TASK_NAME;

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
esp_err_t NetworkConnectionAgent::setup(void) {
    esp_err_t ret = ESP_OK;

    this->update_queue = xQueueCreate(10, sizeof(net_conn_agent_event_t));
    this->connection.setup();
    this->state_machine.setup(&this->connection);

    ESP_GOTO_ON_ERROR(
        esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &NetworkConnectionAgent::sOnEthEvent, this),
        err, TAG, "Failed to register ETH_EVENT handler"
    );

    ESP_GOTO_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &NetworkConnectionAgent::sOnWifiEvent, this),
        err, TAG, "Failed to register WIFI_EVENT handler"
    );
    
    ESP_GOTO_ON_ERROR(
        esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &NetworkConnectionAgent::sOnIPEvent, this),
        err, TAG, "Failed to register IP_EVENT handler"
    );

err:
    return ret;
}

//*****************************************************************************
void NetworkConnectionAgent::connect(void) {
    net_conn_agent_event_t next_event = e_nc_event_net_connect;
    event_callback(e_net_agent_connecting, event_callback_context);
    xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
}

//*****************************************************************************
void NetworkConnectionAgent::disconnect(void) {
    net_conn_agent_event_t next_event = e_nc_event_net_disconnect;
    event_callback(e_net_agent_disconnecting, event_callback_context);
    xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
}

//*****************************************************************************
void NetworkConnectionAgent::restart(void) {
    this->disconnect();
    // sleep 2 seconds
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    this->connect();
}

//*****************************************************************************
void NetworkConnectionAgent::onIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got IPEvent");
    ip_event_t event = (ip_event_t) event_id;
    net_conn_agent_event_t next_event;
    switch (event) {
        case IP_EVENT_STA_GOT_IP:
        case IP_EVENT_ETH_GOT_IP:
            ESP_LOGI(TAG, "Connected with IP Address");
            next_event = e_nc_event_net_connected;
            event_callback(e_net_agent_connected, event_callback_context);
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            break;
        case IP_EVENT_STA_LOST_IP:
        case IP_EVENT_ETH_LOST_IP:
            ESP_LOGI(TAG, "Disconnected from IP Address");
            next_event = e_nc_event_net_disconnected;
            event_callback(e_net_agent_disconnected, event_callback_context);
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            break;
        default:
            ESP_LOGI(TAG, "Got wifi event %d", event);
            break;
    }
}

//*****************************************************************************
void NetworkConnectionAgent::onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got ethEvent");
    eth_event_t event = (eth_event_t) event_id;
    net_conn_agent_event_t next_event;
    switch (event) {
        case ETHERNET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Ethernet Connected");
            next_event = e_nc_event_net_connected;
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Disconnected");
            next_event = e_nc_event_net_disconnected;
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            break;
        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet Started");
            next_event = e_nc_event_net_connecting;
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            break;
        case ETHERNET_EVENT_STOP:
            next_event = e_nc_event_net_disconnected;
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            ESP_LOGI(TAG, "Ethernet Stopped");
            break;
        default:
            ESP_LOGI(TAG, "Got eth event %d", event);
            break;
    }
}

//*****************************************************************************
void NetworkConnectionAgent::onWifiEvent(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got wifiEvent");
    wifi_event_t event = (wifi_event_t) event_id;
    net_conn_agent_event_t next_event;
    switch (event) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Wifi Started");
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "Wifi Stopped");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Wifi Connected");
            next_event = e_nc_event_net_connected;
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            next_event = e_nc_event_net_disconnected;
            xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
            ESP_LOGI(TAG, "Wifi Disconnected");
            break;
        default:
            ESP_LOGI(TAG, "Got wifi event %d", event);
            break;
    }
}

//******************************************************************************
void NetworkConnectionAgent::try_connecting(const char* interface) {
    net_conn_agent_event_t next_event = e_nc_event_net_connecting;
    xQueueSend(this->update_queue, &next_event, portMAX_DELAY);
}


//******************************************************************************
// We threat both connection as one.  We will have a single connection state
// machine that will monitor both connections.  We expect both to be up at all
// time.  If we lose one, and the other is still up, we stay in the connected
// state.  If we lose both, then we go to the disconnected state.
//
// The network connection state machine will have the following states:
//
//  - disabled: both ethernet and wifi are disabled
//  - connecting: either ethernet or wifi is connecting
//  - connected: either ethernet or wifi is connected
//  - disconnected: either ethernet or wifi is disconnected
//  - testing_connection: either ethernet or wifi is testing the connection
//  - error: either ethernet or wifi is in error
//
// triggers:
//  - enable_ethernet: enable ethernet
//  - disable_ethernet: disable ethernet
//  - enable_wifi: enable wifi
//  - disable_wifi: disable wifi
//  - ethernet_connected: ethernet is connected
//  - ethernet_disconnected: ethernet is disconnected
void NetworkConnectionAgent::taskFunction(void)
{
    ESP_LOGI(TAG, "Starting connection state machine task");

    net_conn_agent_event_t event;

    // 15 minute timer
    TickType_t queue_timeout = 15 * 60 * 1000 / portTICK_PERIOD_MS;

    while (1)
    {
        // TODO if we are connected, or expected to be connected, we should
        // ping every 15 minutes to ensure the connection is still good.
        // If the connection goes bad, we should loose the mqtt.  So maybe
        // we should have an event to check the connection (using ping) and
        // a state that is checking the connection.  If the connection is
        // good, we go back to the connected state.  If the connection is
        // bad, we go to the error state?  We should also check if the wifi
        // is bad, take it down and try reconnecting it?
        if (xQueueReceive(this->update_queue, &event, queue_timeout) == pdTRUE) {
            this->state_machine.handle_event(event);
        } else {
            // We timed out, if connected, do a ping to ensure connection is still good.
            // If not connected, then we will try to connect.
            ESP_LOGI(TAG, "Network connection state machine timed out, ping");
            this->state_machine.handle_event(e_nc_event_timeout);
        }
    }
}