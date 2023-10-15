#include "MqttAgent.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "network_transport.h"

static const char *TAG = "mqtt_agent";

// extern char* client_cert;
// extern char* client_key;
// extern char* root_ca;
// extern char* thing_id;

/* Network event group bit definitions */
#define NET_CONNECTED_BIT              ( 1 << 0 )
#define NET_DISCONNECTED_BIT           ( 1 << 1 )
#define MQTT_AGENT_CONNECTED_BIT       ( 1 << 2 )
#define MQTT_AGENT_DISCONNECTED_BIT    ( 1 << 3 )

//******************************************************************************
esp_err_t MqttAgent::setup(ThingConfig* thing_config) {
    struct timespec tp;

    this->thing_config = thing_config;
    this->event_group = xEventGroupCreate();

    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing Mqtt Agent");

    ESP_GOTO_ON_ERROR(
        esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &MqttAgent::sOnEthEvent, this),
        err, TAG, "Failed to register ETH_EVENT handler"
    );

    ESP_GOTO_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &MqttAgent::sOnWifiEvent, this),
        err, TAG, "Failed to register WIFI_EVENT handler"
    );
    
    ESP_GOTO_ON_ERROR(
        esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &MqttAgent::sOnIPEvent, this),
        err, TAG, "Failed to register IP_EVENT handler"
    );

    // client_cert = (char*)thing_config->get_certificate_pem();
    // client_key = (char*)thing_config->get_private_key();
    // root_ca = (char*)thing_config->get_root_ca();
    // thing_id = (char*)thing_config->get_thing_name();
    
    // ESP_LOGI(TAG, "%s\n%s\n%s\n", client_cert, client_key, root_ca);
    this->mqtt_connection.initialize(this->thing_config);
    this->mqtt_context.initialize(this->mqtt_connection.get_network_context(), &MqttAgent::sOn_mqtt_pubsub_event, (void*)this);

    ( void ) clock_gettime( CLOCK_REALTIME, &tp );
    srand( tp.tv_nsec );

err:

    return ret;
}

//******************************************************************************
// Make this function synchronous and wait for the ack before returning?
esp_err_t MqttAgent::subscribe(const char *topic, mqttCallbackFn callback, void* context) {
    esp_err_t ret = ESP_OK;
    MQTTStatus_t mqtt_status;
    MQTTSubscribeInfo_t sub_info[1]; // MQTTSubscribe takes a list.
    uint16_t packet_id = 0;

    memset(&sub_info, 0x00, sizeof(MQTTSubscribeInfo_t));

    /* Start with everything at 0. */
    // ( void ) memset( ( void * ) pGlobalSubscriptionList, 0x00, sizeof( pGlobalSubscriptionList ) );

    /* This example subscribes to only one topic and uses QOS1. */
    sub_info[0].qos = MQTTQoS1;
    sub_info[0].pTopicFilter = topic;
    sub_info[0].topicFilterLength = strlen(topic);

    /* Generate packet identifier for the SUBSCRIBE packet. */
    packet_id = MQTT_GetPacketId( &this->mqtt_context );

    /* Send SUBSCRIBE packet. */
    mqtt_status = MQTT_Subscribe( &this->mqtt_context,
                                 sub_info,
                                 1,
                                 packet_id );

    if( mqtt_status != MQTTSuccess )
    {
        ESP_LOGE( TAG, "Failed to send SUBSCRIBE packet to broker with error = %s.",
                    MQTT_Status_strerror( mqtt_status ) );
        ret = ESP_FAIL;
    }
    else
    {
        ESP_LOGI( TAG, "SUBSCRIBE sent for topic %s to broker.\n\n", topic );
    }

    // TODO wait for the acknowledgement.

    return ret;
}

esp_err_t MqttAgent::unsubscribe(const char *topic) {
    return ESP_OK;
}

esp_err_t MqttAgent::publish_message(const char *topic, const char *payload, uint8_t retry_count) {
    esp_err_t ret = ESP_OK;
    MQTTStatus mqtt_status = MQTTSuccess;
    uint16_t packet_id;

    MQTTPublishInfo_t packet;
    memset(&packet, 0x00, sizeof(MQTTPublishInfo_t));

    packet.qos = MQTTQoS1;
    packet.pTopicName = topic;
    packet.topicNameLength = strlen(topic);
    packet.pPayload = (uint8_t*)payload;
    packet.payloadLength = strlen(payload);
    packet_id = MQTT_GetPacketId( &this->mqtt_context );

    mqtt_status = MQTT_Publish( &this->mqtt_context, &packet, packet_id );
    if( mqtt_status != MQTTSuccess )
    {
        ESP_LOGE( TAG, "Failed to send PUBLISH packet to broker with error = %s.",
                    MQTT_Status_strerror( mqtt_status ) );
        ret = ESP_FAIL;
    }
    else
    {
        ESP_LOGI( TAG, "PUBLISH sent for topic %s to broker.\n\n", topic );
    }

    return ret;
}

//******************************************************************************
void MqttAgent::on_mqtt_pubsub_event(
    struct MQTTContext * context,
    struct MQTTPacketInfo * packet_info,
    struct MQTTDeserializedInfo * deserialized_info
) {
    if ((packet_info->type & 0xF0U) == MQTT_PACKET_TYPE_PUBLISH) {
        assert( deserialized_info->pPublishInfo != NULL );
        ESP_LOGI(TAG, "Got publish event");
        // subscription_handler->handle_publish( packet_info, deserialized_info );
    } else {
        ESP_LOGI(TAG, "Got other event");
        // pubsub_handler->handle_packet( packet_info, deserialized_info );
    }
}

//******************************************************************************
esp_err_t MqttAgent::process_mqtt_loop(void) {
    MQTTStatus_t mqtt_status = MQTTSuccess;

    // Only stay in the loop if there is more incoming data.  This under the hood
    // ends up calling our subscribe callback.
    do {
        mqtt_status = MQTT_ProcessLoop( &this->mqtt_context );

        if (mqtt_status == MQTTNeedMoreBytes) {
            // Need to wait a bit between calls to MQTT_ProcessLoop().
            // This is per documentation in the header files where MQTTNeedMoreBytes
            // is defined.
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    } while (mqtt_status == MQTTNeedMoreBytes);

    if (mqtt_status != MQTTSuccess) {
        ESP_LOGE(TAG, "MQTT_ProcessLoop() failed with status %s.",
                 MQTT_Status_strerror(mqtt_status));
        return ESP_FAIL;
    }

    return ESP_OK;
}

//******************************************************************************
void MqttAgent::taskFunction(void) {
    esp_err_t ret = ESP_OK;
    bool connected = false;

    // Init mqtt

    while (1) {
        /* Wait for the device to be connected to WiFi and be disconnected from
         * MQTT broker. */
        xEventGroupWaitBits( this->event_group,
                             NET_CONNECTED_BIT, // | MQTT_AGENT_DISCONNECTED_BIT,
                             pdFALSE,
                             pdTRUE,
                             portMAX_DELAY );

        // Connect to the broker
        if (!connected) {
            ret = this->mqtt_connection.connect_with_retries(&this->mqtt_context, 10);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to connect to MQTT broker");
                continue;
            }
            
            ESP_LOGI(TAG, "Connected to mqtt broker");
            connected = true;
            if (this->mqtt_connection.is_broker_session_present()) {
                ESP_LOGI(TAG, 
                    "An MQTT session with the broker is re-esablished. "
                    "Resending unacked publishes"
                );
                // pubsub.handle_publish_resend();
            } else {
                ESP_LOGI(TAG, 
                    "A clean MQTT connection is established. "
                    "Cleaning up all stored outgoing publish requests"
                );
                // pubsub.cleanup_outgoing_publishes();
            }

            this->event_callback(e_mqtt_agent_connected, this->event_callback_context);

            // This should be in the main app state machine logic.
            // but for testing right now.
            char topic[32];
            memset(topic, 0x00, 32);
            snprintf(topic, 32, "%s/ledstate", this->thing_config->get_thing_name());
            this->subscribe(topic, NULL, NULL);

            memset(topic, 0x00, 32);
            snprintf(topic, 32, "%s/%s", this->thing_config->get_thing_name(), "latest");
            this->publish_message(topic, "{}", 0);

            // xEventGroupSetBits( this->event_group, MQTT_AGENT_CONNECTED_BIT );
            
        }

        // poll mqtt for data
        //ulTaskNotifyTake
        ret = this->process_mqtt_loop();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to process mqtt loop");
            this->event_callback(e_mqtt_agent_disconnected, this->event_callback_context);
            this->mqtt_connection.disconnect(&this->mqtt_context);
            connected = false;
        }


        // delay 5 seconds
        vTaskDelay(5000 / portTICK_PERIOD_MS);
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

