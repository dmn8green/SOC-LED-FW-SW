#include "IotHeartbeat.h"
#include "MN8StateMachine.h"

#include "Utils/FuseMacAddress.h"
#include "Utils/KeyStore.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_eth_com.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rev.h"

static const char* TAG = "iot_heartbeat";


//******************************************************************************
esp_err_t IotHeartbeat::setup(
    MqttAgent* mqtt_context, 
    ThingConfig* thing_config,
    MN8StateMachine *state_machine
) {
    this->mqtt_context = mqtt_context;
    this->thing_config = thing_config;
    this->state_machine = state_machine;

    KeyStore key_store;
    key_store.openKeyStore("config", e_ro);
    key_store.getKeyValue("heartbeat_frequency", this->heartbeat_frequency);

    return ESP_OK;
}

void IotHeartbeat::send_heartbeat(void) {
    ESP_LOGI(TAG, "Sending heartbeat");

    char topic[64] = {0};
    char payload[128] = {0};

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *) topic, 64, "%s/heartbeat", mac_address);
    snprintf((char *) payload, sizeof(payload), 
      R"({
        "version":"%d.%d.%d",
        "current_state":"%s",
      })", 
      VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
      this->state_machine->get_current_state_name()
    );

    this->mqtt_context->publish_message(topic, payload, 3);
}

//******************************************************************************
void IotHeartbeat::taskFunction(void) {
    ESP_LOGI(TAG, "Starting IotHeartbeat task");
    while (1) {
        vTaskDelay((this->heartbeat_frequency * 60 * 1000) / portTICK_PERIOD_MS);
        if (this->mqtt_context->is_connected()) {
            this->send_heartbeat();
        }
    }
}