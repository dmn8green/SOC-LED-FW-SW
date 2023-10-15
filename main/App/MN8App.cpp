//*****************************************************************************
/**
 * @file MN8App.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief MN8App class implementation
 * @version 0.1
 * @date 2023-08-25
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//*****************************************************************************

#include "MN8App.h"

#include "pin_def.h"

#include "ArduinoJson.h"

#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_vfs_fat.h"
#include "esp_check.h"


#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

const char* TAG = "MN8App";

/*
The network agent state machine goals is to keep the network up and running
at all cost.  If after a while it can't connect it should notify the app, via
a callback, that the network is down.  The app can then decide what to do.

The mqtt agent is responsible for connecting to the mqtt broker and keeping
the connection alive.  If the connection is lost, it should notify the app
via a callback.  The app can then switch to the error state waiting for the
network to come back up or to be told that the network is very down.

The main app state machine is responsible for keeping the leds up to date
with the current state of the system.  It should also be responsible for
switching the leds to the error state if the network is down or the mqtt
connection is down.
*/

//*****************************************************************************
static esp_err_t initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    return err;
}

//*****************************************************************************
// This needs to be moved to a different file.
typedef void (* handleIncomingPublishCallback_t)( const char* pTopicName,
                                                    uint16_t topicNameLength,
                                                    const char* pPayload,
                                                    size_t payloadLength,
                                                  uint16_t packetIdentifier );
extern "C" handleIncomingPublishCallback_t handleIncomingPublishCallback;

void handleIncomingPublish( const char* pTopicName,
                            uint16_t topicNameLength,
                            const char* pPayload,
                            size_t payloadLength,
                            uint16_t packetIdentifier )
{
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, pPayload, payloadLength);
    if (error) {
        ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
        return;
    }

    JsonObject root = doc.as<JsonObject>();
    if (root.isNull()) {
        ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
        return;
    }

    auto& app = MN8App::instance();
    if (root.containsKey("port1")) {
        JsonObject port1 = root["port1"];
        if (port1.containsKey("state")) {
            const char* state = port1["state"];
            int charge_percent = port1["charge_percent"];
            ESP_LOGI(TAG, "port 1 new state : %s", state);
            app.get_context().get_led_task_0().set_state(state, charge_percent);
        }
    }

    if (root.containsKey("port2")) {
        JsonObject port2 = root["port2"];
        if (port2.containsKey("state")) {
            const char* state = port2["state"];
            int charge_percent = port2["charge_percent"];
            ESP_LOGI(TAG, "port 2 new state : %s", state);
            app.get_context().get_led_task_1().set_state(state, charge_percent);
        }
    }

    ESP_LOGI(TAG, "Incoming publish received : %.*s", payloadLength, pPayload);
    // ESP_LOGI(TAG, "Incoming publish received : %s", (char*) root["state"]);
}

//*****************************************************************************
/**
 * @brief Setup the MN8App.
 * 
 * This function will setup the wifi and ethernet connections.
 * It will also setup the led strip.
 * 
 * @return esp_err_t 
 */
esp_err_t MN8App::setup(void) {
    esp_err_t ret = ESP_OK;
    auto& thing_config = this->context.get_thing_config();

    // Do this as early as possible to have feedback.
    ESP_GOTO_ON_ERROR(this->setup_and_start_led_tasks(), err, TAG, "Failed to setup and start led tasks");;

    // This has to be done early as well.
    ESP_GOTO_ON_ERROR(initialize_nvs(), err, TAG, "Failed to initialize NVS");

    // We need this to have our event loop.  Without this, we can't get the
    // network events or any other events.
    ESP_GOTO_ON_ERROR(esp_event_loop_create_default(), err, TAG, "Failed to create event loop");

    this->context.get_network_connection_agent().register_event_callback(this->sOn_network_event, this);
    this->context.get_mqtt_agent().register_event_callback(this->sOn_mqtt_event, this);

    ESP_GOTO_ON_ERROR(
        context.get_network_connection_agent().setup(), 
        err, TAG, "Failed to setup connection state machine"
    );

    // Moved to state machine.
    this->context.get_network_connection_agent().start();
    
    // MQTT starter config stuff. Only start mqtt stack if thing is configured.
    thing_config.load();
    if (thing_config.is_configured()) {
        this->context.get_mqtt_agent().setup(&thing_config);
        handleIncomingPublishCallback = handleIncomingPublish;
        this->context.get_mqtt_agent().start();
    }

    this->context.get_network_connection_agent().start();
    this->context.get_mqtt_agent().start();


    this->message_queue = xQueueCreate(10, sizeof(mn8_event_t));

err:
    return ret;
}

//*****************************************************************************
/**
 * @brief Setup and start the LED tasks.
 * 
 * @return esp_err_t 
 */
esp_err_t MN8App::setup_and_start_led_tasks(void) {
    esp_err_t ret = ESP_OK;
    auto & led_task_0 = this->context.get_led_task_0();
    auto & led_task_1 = this->context.get_led_task_1();

    ESP_GOTO_ON_ERROR(led_task_0.setup(0, RMT_LED_STRIP0_GPIO_NUM, HSPI_HOST), err, TAG, "Failed to setup led task 0");
    ESP_GOTO_ON_ERROR(led_task_0.start(), err, TAG, "Failed to start led task 0");

    ESP_GOTO_ON_ERROR(led_task_1.setup(1, RMT_LED_STRIP1_GPIO_NUM, VSPI_HOST), err, TAG, "Failed to setup led task 1");
    ESP_GOTO_ON_ERROR(led_task_1.start(), err, TAG, "Failed to start led task 1");

err:
    return ret;
}

//*****************************************************************************
// callback for the network connection state machine.
void MN8App::on_network_event(NetworkConnectionAgent::event_t event) {
    ESP_LOGI(TAG, "Network event : %d", event);
    switch (event) {
        case NetworkConnectionAgent::event_t::e_net_agent_connection_error:
            break;
        case NetworkConnectionAgent::event_t::e_net_agent_connecting:
            break;
        case NetworkConnectionAgent::event_t::e_net_agent_connected:
            break;
        case NetworkConnectionAgent::event_t::e_net_agent_disconnected:
            break;
        case NetworkConnectionAgent::event_t::e_net_agent_disconnecting:
            break;
        default:
            break;
    }
}

//*****************************************************************************
// callback for the mqtt state machine.
void MN8App::on_mqtt_event(MqttAgent::event_t event) {
    ESP_LOGI(TAG, "MQTT event : %d", event);
    switch (event) {
        case MqttAgent::event_t::e_mqtt_agent_connected:
            break;
        case MqttAgent::event_t::e_mqtt_agent_disconnected:
            break;
        default:
            break;
    }
}


//*****************************************************************************
/**
 * @brief Loop function for the MN8App.
 * 
 * This function will be called from the main loop.
 * It will handle the state machine logic?? TBD
 * 
 */
void MN8App::loop(void) {
    while(1) {
        // check the network connection state machine.
        // check the mqtt state machine.

        vTaskDelay(5000 / portTICK_PERIOD_MS);
        // ESP_LOGI(__func__, "Hello world!");
    }
}

