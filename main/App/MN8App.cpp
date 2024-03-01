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
#include "LED/Led.h"
#include "pin_def.h"
#include "rev.h"

#include "Utils/FuseMacAddress.h"
#include "Utils/Colors.h"
#include "Utils/KeyStore.h"

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
/**
 * @brief Configure GPIO for demo board.
 * 
 * This is to work around the being held in reset on boot because of a conflict
 * with the eth dev kit board.
*/
static esp_err_t configure_gpio_for_demo(void) {
    #define GPIO_BIT_MASK  ((1ULL<<GPIO_NUM_33)) 

	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_BIT_MASK;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);

    gpio_set_level(GPIO_NUM_33, 1);

    return ESP_OK;
}

static void configure_gpio_for_light_sensor(void) {
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1ULL<<LIGHT_SENSOR_GPIO_NUM;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);
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
    KeyStore key_store;

    bool reseted_due_to_network_error = false;
    if (esp_reset_reason() == ESP_RST_SW) {
        reseted_due_to_network_error = true;
    }

    auto& thing_config = this->context.get_thing_config();

    configure_gpio_for_light_sensor();

    this->context.setup();

    // Do this as early as possible to have feedback.
    ESP_GOTO_ON_ERROR(this->setup_and_start_led_tasks(reseted_due_to_network_error), err, TAG, "Failed to setup and start led tasks");;

    // This has to be done early as well.
    ESP_GOTO_ON_ERROR(initialize_nvs(), err, TAG, "Failed to initialize NVS");

    // We need this to have our event loop.  Without this, we can't get the
    // network events or any other events.
    ESP_GOTO_ON_ERROR(esp_event_loop_create_default(), err, TAG, "Failed to create event loop");

    configure_gpio_for_demo();

    this->context.get_network_connection_agent().register_event_callback(this->sOn_network_event, this);
    this->context.get_mqtt_agent().register_event_callback(this->sOn_mqtt_event, this);
    this->context.get_mqtt_agent().register_handle_incoming_mqtt(this->sOn_incoming_mqtt, this);

    ESP_GOTO_ON_ERROR(
        context.get_network_connection_agent().setup(), 
        err, TAG, "Failed to setup connection state machine"
    );

    // Moved to state machine.
    this->context.get_network_connection_agent().start();
    
    // MQTT starter config stuff. Only start mqtt stack if thing is configured.
    if (thing_config.is_configured()) {
        this->context.get_mqtt_agent().setup(&thing_config);
        this->context.get_iot_thing().setup(&thing_config, &this->context.get_mqtt_agent());
        this->context.get_mqtt_agent().start();

        this->context.get_iot_heartbeat().setup(&this->context, &thing_config, &this->state_machine);
        this->context.get_iot_heartbeat().start();
    }

    this->context.get_network_connection_agent().start();
    this->state_machine.setup(&this->context);
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
esp_err_t MN8App::setup_and_start_led_tasks(bool disable_connecting_leds) {
    esp_err_t ret = ESP_OK;
    auto & led_task_0 = this->context.get_led_task_0();
    auto & led_task_1 = this->context.get_led_task_1();
    auto& site_config = this->context.get_site_config();

    ESP_LOGI(TAG, "LED length : %d", site_config.get_led_length());
    int led_count = site_config.get_led_length() == LED_FULL_SIZE ? LED_STRIP_PIXEL_COUNT : LED_STRIP_SHORT_PIXEL_COUNT;
    ESP_LOGI(TAG, "LED count : %d", led_count);

    ESP_GOTO_ON_ERROR(led_task_0.setup(0, RMT_LED_STRIP0_GPIO_NUM, HSPI_HOST, led_count, disable_connecting_leds), err, TAG, "Failed to setup led task 0");
    ESP_GOTO_ON_ERROR(led_task_0.start(), err, TAG, "Failed to start led task 0");

    ESP_GOTO_ON_ERROR(led_task_1.setup(1, RMT_LED_STRIP1_GPIO_NUM, VSPI_HOST, led_count, disable_connecting_leds), err, TAG, "Failed to setup led task 1");
    ESP_GOTO_ON_ERROR(led_task_1.start(), err, TAG, "Failed to start led task 1");

err:
    return ret;
}

static char topic[64] = {0};
static char payload[2048] = {0};

//*****************************************************************************
void MN8App::on_incoming_mqtt( 
    const char* pTopicName,
    uint16_t topicNameLength,
    const char* pPayload,
    size_t payloadLength,
    uint16_t packetIdentifier
) {
    ESP_LOGI(TAG, "Incoming publish received : %.*s %.*s", topicNameLength, pTopicName, payloadLength, pPayload);
    KeyStore key_store;

    StaticJsonDocument<512> doc;
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

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);
    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));

    if (strnstr(pTopicName, "ledstate", topicNameLength) != NULL) {
        last_received_led_state = Time::instance().upTimeS();

        if (root.containsKey("night_mode")) {
            bool night_mode = root["night_mode"];
            this->context.set_night_mode(night_mode);
            Colors::instance().setMode(night_mode ? LED_INTENSITY_LOW : LED_INTENSITY_HIGH);
        }

        if (root.containsKey("port0")) {
            JsonObject port0 = root["port0"];
            if (port0.containsKey("state")) {
                const char* state = port0["state"];
                int charge_percent = port0["charge_percent"];
                ESP_LOGI(TAG, "port 0 new state : %s", state);
                this->get_context().get_led_task_0().set_state(state, charge_percent);
            }
        }

        if (root.containsKey("port1")) {
            JsonObject port1 = root["port1"];
            if (port1.containsKey("state")) {
                const char* state = port1["state"];
                int charge_percent = port1["charge_percent"];
                ESP_LOGI(TAG, "port 1 new state : %s", state);
                this->get_context().get_led_task_1().set_state(state, charge_percent);
            }
        }

        memcpy(payload, pPayload, payloadLength);
        this->get_context().get_iot_thing().ack_led_state_change(payload);

        // Right here we could send a message to state machine to pet a watchdog
        // in the state maching if watch dog hasn't been pet in a while we would
        // go to proxy connection lost.
    } else if (strnstr(pTopicName, "ping", topicNameLength) != NULL) {
        ESP_LOGI(TAG, "ping received");
        this->get_context().get_iot_thing().send_pong(
            this->state_machine.get_current_state_name(),
            this->get_context().get_led_task_0().get_state_as_string(),
            this->get_context().get_led_task_1().get_state_as_string(),
            this->get_context().is_night_mode(),
            this->get_context().has_night_sensor()
        );
    } else if (strnstr(pTopicName, "reboot", topicNameLength) != NULL) {
        ESP_LOGI(TAG, "reboot received");
        esp_restart();
    } else if (strnstr(pTopicName, "set-config", topicNameLength) != NULL) {
        key_store.openKeyStore("config", e_rw);

        if (root.containsKey("heartbeat_frequency")) {
            uint16_t heartbeat_frequency = root["heartbeat_frequency"];
            ESP_LOGI(TAG, "heartbeat_frequency : %d", heartbeat_frequency);
            key_store.setKeyValue("heartbeat_frequency", heartbeat_frequency);
            this->context.get_iot_heartbeat().set_heartbeat_frequency(heartbeat_frequency);
        }
    } else if (strnstr(pTopicName, "get-config", topicNameLength) != NULL) {
        key_store.openKeyStore("config", e_ro);

        uint16_t heartbeat_frequency = this->context.get_iot_heartbeat().get_heartbeat_frequency();

        snprintf((char *) topic, 64, "%s/config", mac_address);
        snprintf(
            payload, sizeof(payload),
            R"({
                "heartbeat_frequency":"%d",
            })",
            heartbeat_frequency
        );

        this->get_context().get_mqtt_agent().publish_message(topic, payload, 3);
    }
}

//*****************************************************************************
// callback for the network connection state machine.
void MN8App::on_network_event(NetworkConnectionAgent::event_t event) {
    ESP_LOGI(TAG, "Network event : %d", event);
    switch (event) {
        case NetworkConnectionAgent::event_t::e_net_agent_connection_error: {
            mn8_event_t event = e_mn8_event_lost_network_connection;
            xQueueSend(this->message_queue, &event, 0);
            break;
        }
        case NetworkConnectionAgent::event_t::e_net_agent_connected: {
            mn8_event_t event = e_mn8_event_net_connected;
            xQueueSend(this->message_queue, &event, 0);
            break;
        }
        case NetworkConnectionAgent::event_t::e_net_agent_disconnected: {
            mn8_event_t event = e_mn8_event_net_disconnected;
            xQueueSend(this->message_queue, &event, 0);
            break;
        }
        default:
            break;
    }
}

//*****************************************************************************
// callback for the mqtt state machine.
void MN8App::on_mqtt_event(MqttAgent::event_t event) {
    ESP_LOGI(TAG, "MQTT event : %d", event);
    switch (event) {
        case MqttAgent::event_t::e_mqtt_agent_connected: {
            mn8_event_t event = e_mn8_event_mqtt_connected;
            xQueueSend(this->message_queue, &event, 0);
            break;
        }
        case MqttAgent::event_t::e_mqtt_agent_disconnected: {
            mn8_event_t event = e_mn8_event_mqtt_disconnected;
            xQueueSend(this->message_queue, &event, 0);
            break;
        }
        default:
            break;
    }
}

//*****************************************************************************
/**
 * @brief Loop function for the MN8App.
 * 
 * This function will be called from the main loop.
 */
void MN8App::loop(void) {
    ESP_LOGI(TAG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Starting state machine");
    mn8_event_t event;
    TickType_t xTicksToWait = 5000/portTICK_PERIOD_MS;
    
    this->state_machine.turn_on();

    bool new_night_mode = false;
    last_received_led_state = Time::instance().upTimeS();

    while(1) {
        if (xQueueReceive(this->message_queue, &event, xTicksToWait) == pdTRUE) {
            ESP_LOGI(TAG, "Event received : %d", event);
            this->state_machine.handle_event(event);
        }

        // Check if we have not received any message from proxy in a bit.
        ESP_LOGI(TAG, "Last received led state : %llu", last_received_led_state);
        ESP_LOGI(TAG, "Now is : %llu", Time::instance().upTimeS());
        ESP_LOGI(TAG, "Timeout : %llu", timeout_no_comm_from_proxy.count());
        if (Time::instance().upTimeS() - last_received_led_state >= timeout_no_comm_from_proxy.count()) {
            // Turn off LED.
            ESP_LOGE(TAG, "No communication from proxy");
            this->context.get_led_task_0().set_state("no_connection", 0);
            this->context.get_led_task_1().set_state("no_connection", 0);
        }

        int lightSensorState = gpio_get_level((gpio_num_t) LIGHT_SENSOR_GPIO_NUM);
        ESP_LOGD(TAG, "Light sensor state : %d", lightSensorState);

        // Accessing the freertos mutex fails when called from the main
        // loop.  So we have to do this here.
        if (this->context.get_thing_config().is_configured()) {
            new_night_mode = lightSensorState == 0;
            if (night_mode != new_night_mode) {
                // WE GOT A TRANSITION WE MUST HAVE A SENSOR            
                this->context.set_has_night_sensor(true);
                night_mode = new_night_mode;
                ESP_LOGI(TAG, "Night mode : %d", night_mode);
                this->context.get_iot_heartbeat().set_night_mode(night_mode);
            }
        }
    }
}

