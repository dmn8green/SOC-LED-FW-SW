#pragma once

#include "IOT/ThingConfig.h"
#include "Utils/NoCopy.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_eth_com.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

class MqttAgent : public NoCopy {
public:
    MqttAgent(void) = default;
    ~MqttAgent(void) = default;

    esp_err_t setup(ThingConfig* thing_config);
    esp_err_t start(void);
    esp_err_t resume(void);
    esp_err_t suspend(void);
    esp_err_t stop(void);
    esp_err_t teardown(void);


private:
    void vTaskMqttHandler(void);
    static void svTaskMqttHandler(void* pvParameters) { ((MqttAgent*)pvParameters)->vTaskMqttHandler(); }

    void onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void sOnEthEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((MqttAgent*)arg)->onEthEvent(event_base, event_id, event_data); }

    void onWifiEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void sOnWifiEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((MqttAgent*)arg)->onWifiEvent(event_base, event_id, event_data); }

    void onIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void sOnIPEvent(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) { ((MqttAgent*)arg)->onIPEvent(event_base, event_id, event_data); }

    ThingConfig* thing_config;
    TaskHandle_t task_handle;
    QueueHandle_t queue;
    EventGroupHandle_t event_group;
};