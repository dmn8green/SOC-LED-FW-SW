#include "MqttAgent.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "mqtt_agent";

/* Network event group bit definitions */
#define NET_CONNECTED_BIT              ( 1 << 0 )
#define NET_DISCONNECTED_BIT           ( 1 << 1 )
#define MQTT_AGENT_CONNECTED_BIT       ( 1 << 2 )
#define MQTT_AGENT_DISCONNECTED_BIT    ( 1 << 3 )

//******************************************************************************
esp_err_t MqttAgent::setup(ThingConfig* thing_config) {
    this->thing_config = thing_config;
    this->event_group = xEventGroupCreate();

    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "Initializing Mqtt Agent");
    ret = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &MqttAgent::sOnEthEvent, this);
    if (ret != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to register IP_EVENT handler");
        return ret;
    }

    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &MqttAgent::sOnWifiEvent, this);
    if (ret != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to register IP_EVENT handler");
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &MqttAgent::sOnIPEvent, this);
    if (ret != ESP_OK) { 
        ESP_LOGE(TAG, "Failed to register IP_EVENT handler");
        return ret;
    }

    return ESP_OK;
}

//******************************************************************************
esp_err_t MqttAgent::start(void) {
    // Start the task
    xTaskCreatePinnedToCore(
        MqttAgent::svTaskMqttHandler, // Function to implement the task
        "mqtt_handler",               // Name of the task
        5000,                   // Stack size in words
        this,                   // Task input parameter
        2,                      // Priority
        &this->task_handle,     // Task handle.
        1);                     // Core 1
    // xTaskCreate(&MqttAgent::svTaskMqttHandler, "mqtt_handler", 4096, this, 5, &this->task_handle);
    return ESP_OK;
}

//******************************************************************************
esp_err_t MqttAgent::resume(void) {
    return ESP_OK;
}

//******************************************************************************
esp_err_t MqttAgent::suspend(void) {
    return ESP_OK;
}

//******************************************************************************
esp_err_t MqttAgent::stop(void) {
    return ESP_OK;
}

//******************************************************************************
esp_err_t MqttAgent::teardown(void) {
    return ESP_OK;
}

extern "C" int aws_iot_demo_main();

//******************************************************************************
void MqttAgent::vTaskMqttHandler(void) {
    while (1) {
        /* Wait for the device to be connected to WiFi and be disconnected from
         * MQTT broker. */
        xEventGroupWaitBits( this->event_group,
                             NET_CONNECTED_BIT, // | MQTT_AGENT_DISCONNECTED_BIT,
                             pdFALSE,
                             pdTRUE,
                             portMAX_DELAY );

        ESP_LOGI("MQTT", "Connected to network");
        aws_iot_demo_main();

        // // Wait for the wifi to be connected, and for the mqtt to be disconnected.

        // // If a connection was previously established, close it to free memory.

        // // Init the backoff algorithm.

        // do {
        //     // Connect to ssl

        //     // On success get socket fd to poll. and connect to the broker.

        //     // Connect to the mqtt broker.

        //     // On error disconnect tls, and backoff for retry.
            
        // } while (no good connection or timed out)

        // // if connected
        // while (mqtt is connected) {
        //     // Wait on the socket fd for data to be received.
        //     // If data avail
        //     //     MQTT process loop
        //     //     ukTaskNotifyTake??? No clue what that is
        //     // If error bit is set
        //     //    Disconnect from the broker
        //     //    Post disconnect event. (using esp_event_post???)
        //     // Delay for 1 
        // }
    }

    vTaskDelete(NULL);
}

void MqttAgent::onIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got IPEvent");
    ip_event_t event = (ip_event_t) event_id;
    switch (event) {
        case IP_EVENT_STA_GOT_IP:
        case IP_EVENT_ETH_GOT_IP:
            ESP_LOGI(TAG, "Connected with IP Address");
             xEventGroupSetBits( this->event_group, NET_CONNECTED_BIT );
            break;
        case IP_EVENT_STA_LOST_IP:
        case IP_EVENT_ETH_LOST_IP:
            ESP_LOGI(TAG, "Disconnected from IP Address");
            xEventGroupClearBits( this->event_group, NET_CONNECTED_BIT );
            break;
        default:
            ESP_LOGI(TAG, "Got wifi event %d", event);
            break;
    }
}


void MqttAgent::onEthEvent(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got ethEvent");
    eth_event_t event = (eth_event_t) event_id;
    switch (event) {
        case ETHERNET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Ethernet Connected");
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Disconnected");
            break;
        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet Started");
            break;
        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            break;
        default:
            ESP_LOGI(TAG, "Got eth event %d", event);
            break;
    }
}


void MqttAgent::onWifiEvent(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Got wifiEvent");
    wifi_event_t event = (wifi_event_t) event_id;
    switch (event) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Wifi Started");
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "Wifi Stopped");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Wifi Connected");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Wifi Disconnected");
            break;
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGI(TAG, "Wifi Authmode Changed");
            break;
        case WIFI_EVENT_STA_WPS_ER_SUCCESS:
            ESP_LOGI(TAG, "Wifi WPS ER Success");
            break;
        case WIFI_EVENT_STA_WPS_ER_FAILED:
            ESP_LOGI(TAG, "Wifi WPS ER Failed");
            break;
        case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
            ESP_LOGI(TAG, "Wifi WPS ER Timeout");
            break;
        case WIFI_EVENT_STA_WPS_ER_PIN:
            ESP_LOGI(TAG, "Wifi WPS ER Pin");
            break;
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "Wifi AP Started");
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG, "Wifi AP Stopped");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "Wifi AP STA Connected");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "Wifi AP STA Disconnected");
            break;
        case WIFI_EVENT_AP_PROBEREQRECVED:
            ESP_LOGI(TAG, "Wifi AP Probe Request Received");
            break;
        case WIFI_EVENT_ACTION_TX_STATUS:
            ESP_LOGI(TAG, "Wifi Action TX Status");
            break;
        case WIFI_EVENT_ROC_DONE:
            ESP_LOGI(TAG, "Wifi ROC Done");
            break;
            
        default:
            ESP_LOGI(TAG, "Got wifi event %d", event);
            break;
    }
}


// There is also a MQTTAgent task and an AgentConnectionTask
// The AgentConnectionTask is responsible for connecting to the broker.

// The agent task stays in a while loop 
// Calls the mqtt loop
// until mqttstatus is no longer successful. 
//      Not sure this make sense. what if the connection is lost?
//      The agentconnectiontask will retry the connection.