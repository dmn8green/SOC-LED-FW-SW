#include "IotHeartbeat.h"

#include "Utils/FuseMacAddress.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_eth_com.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "iot_heartbeat";

//******************************************************************************
esp_err_t IotHeartbeat::setup(MqttAgent* mqtt_context, ThingConfig* thing_config) {
    this->mqtt_context = mqtt_context;
    this->thing_config = thing_config;

    return ESP_OK;
}

//******************************************************************************
void IotHeartbeat::taskFunction(void) {
    ESP_LOGI(TAG, "Starting IotHeartbeat task");
    char topic[64] = {0};
    char payload[4] = {0};

    while (1) {
        vTaskDelay(15*60*1000 / portTICK_PERIOD_MS);
        if (this->mqtt_context->is_connected()) {
            ESP_LOGI(TAG, "Sending heartbeat");

            char mac_address[13] = {0};
            get_fuse_mac_address_string(mac_address);

            memset(topic, 0, sizeof(topic));
            memset(payload, 0, sizeof(payload));
            snprintf((char *) topic, 64, "%s/heartbeat", mac_address);
            snprintf((char *) payload, sizeof(payload), "{}");

            this->mqtt_context->publish_message(topic, payload, 3);
        }
    }
}