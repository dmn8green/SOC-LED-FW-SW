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
    MN8Context* mn8_context, 
    ThingConfig* thing_config,
    MN8StateMachine *state_machine
) {
    this->mn8_context = mn8_context;
    this->thing_config = thing_config;
    this->state_machine = state_machine;

    KeyStore key_store;
    key_store.openKeyStore("config", e_ro);
    key_store.getKeyValue("heartbeat_frequency", this->heartbeat_frequency);

    this->message_queue = xQueueCreate(10, sizeof(iot_heartbeat_command_t));

    return ESP_OK;
}

//******************************************************************************
void IotHeartbeat::send_heartbeat(void) {
    ESP_LOGI(TAG, "Sending heartbeat");
    if (this->mn8_context->get_mqtt_agent().is_connected()) {
        this->mn8_context->get_iot_thing().send_heartbeat(
            this->state_machine->get_current_state_name(),
            this->mn8_context->get_led_task_0().get_state_as_string(),
            this->mn8_context->get_led_task_1().get_state_as_string(),
            this->mn8_context->is_night_mode(),
            this->mn8_context->has_night_sensor()
        );
        
        if (this->mn8_context->has_night_sensor()) {
            this->mn8_context->get_iot_thing().send_night_mode(this->mn8_context->is_night_mode());
        }
    }
}

void IotHeartbeat::set_night_mode(bool night_mode) {
    this->mn8_context->set_night_mode(night_mode);
    iot_heartbeat_command_t command = e_iot_night_mode_changed;
    xQueueSend(this->message_queue, &command, 0);
}

//******************************************************************************
void IotHeartbeat::taskFunction(void) {
    ESP_LOGI(TAG, "Starting IotHeartbeat task");
    iot_heartbeat_command_t command;
    TickType_t wait_time = this->heartbeat_frequency * 60 * 1000 / portTICK_PERIOD_MS;

    while (1) {
        ESP_LOGI(TAG, "Waiting for command or timeout");
        if (xQueueReceive(this->message_queue, &command, wait_time) == pdTRUE) {
            switch (command) {
                case e_iot_night_mode_changed:
                case e_iot_heartbeat_send:
                    this->send_heartbeat();
                    break;
                default:
                    ESP_LOGE(TAG, "Unknown command received");
                    break;
            }
        } else {
            this->send_heartbeat();
        }
    }
}