#include "IotThing.h"

#include "App/Configuration/ThingConfig.h"
#include "App/MqttAgent/MqttAgent.h"

#include "Utils/FuseMacAddress.h"

#include "rev.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

// NOTE:
// PUBLISH and SUBSCRIBE CODE SHOULD BE HERE, NOT IN THE AGENT.
// There should be an MN8Thing that derive from IOTThing that has the publish and subscribe code.
// The MN8Thing should be the one that is used in the MN8StateMachine.

static const char* TAG = "iot_thing";

static char topic[64] = {0};
static char payload[4096] = {0};


esp_err_t IotThing::setup(ThingConfig* thing_config, MqttAgent* mqtt_agent) {
    this->mqtt_agent = mqtt_agent;
    this->thing_config = thing_config;

    return ESP_OK;
}

esp_err_t IotThing::send_night_mode(bool night_mode) {
    ESP_LOGI(TAG, "Sending light sensor reading");

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *) topic, sizeof(topic), "%s/light_sensor", mac_address);
    snprintf((char *) payload, sizeof(payload), 
      R"({
        "night_mode": %s
      })", 
      night_mode ? "true" : "false"
    );

    this->mqtt_agent->publish_message(topic, payload, 3);

    return ESP_OK;
}

esp_err_t IotThing::send_heartbeat(
    const char* current_state,
    const char* led1_state,
    const char* led2_state,
    bool night_mode,
    bool has_night_sensor
) {
    ESP_LOGI(TAG, "Sending heartbeat");

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *) topic, sizeof(topic), "%s/heartbeat", mac_address);
    snprintf((char *) payload, sizeof(payload), 
        R"({
            "version":"%d.%d.%d",
            "current_state":"%s",
            "led1_state":"%s",
            "led2_state":"%s",
            "night_mode": %s,
            "has_night_sensor": %s
        })", 
        VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
        current_state,
        led1_state,
        led2_state,
        night_mode ? "true" : "false",
        has_night_sensor ? "true" : "false"
    );

    this->mqtt_agent->publish_message(topic, payload, 3);

    return ESP_OK;
}

esp_err_t IotThing::ack_led_state_change(const char* received_payload) {
    ESP_LOGI(TAG, "Sending ack for led state change");

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    snprintf((char *) topic, sizeof(topic), "%s/ack_ledstate", mac_address);

    this->mqtt_agent->publish_message(topic, received_payload, 3);

    return ESP_OK;
}

esp_err_t IotThing::send_pong(
    const char* current_state,
    const char* led1_state,
    const char* led2_state,
    bool night_mode,
    bool has_night_sensor
) {
    ESP_LOGI(TAG, "Sending pong");

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *) topic, sizeof(topic), "%s/pong", mac_address);
    snprintf((char *) payload, sizeof(payload), 
        R"({
            "version":"%d.%d.%d",
            "current_state":"%s",
            "led1_state":"%s",
            "led2_state":"%s",
            "night_mode": %s,
            "has_night_sensor": %s
        })", 
        VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
        current_state,
        led1_state,
        led2_state,
        night_mode ? "true" : "false",
        has_night_sensor ? "true" : "false"
    );

    this->mqtt_agent->publish_message(topic, payload, 3);

    return ESP_OK;
}