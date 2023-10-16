#include "MqttContext.h"
#include "App/MqttAgent/MqttConfig.h"

#include "network_transport.h"

#include <memory.h>

static const char * TAG = "MqttContext";

//******************************************************************************
MqttContext::MqttContext(void) {
    memset( &this->outgoing_records, 0x00, sizeof( MQTTPubAckInfo_t ) * OUTGOING_PUBLISH_RECORD_LEN );
    memset( &this->incoming_records, 0x00, sizeof( MQTTPubAckInfo_t ) * INCOMING_PUBLISH_RECORD_LEN );
    this->buffer = (uint8_t*)malloc( NETWORK_BUFFER_SIZE );
    memset( this->buffer, 0x00, NETWORK_BUFFER_SIZE );
    
    memset( (void*) &this->mqtt_context, 0x00, sizeof( MQTTContext_t ) );

    ESP_LOGI(TAG, "!!!!!!!! mn8context: %p", this);
    this->mqtt_context.mn8_context = this;
}

//******************************************************************************
esp_err_t MqttContext::initialize(NetworkContext_t * network_context, EventCallback_t callback, void * callback_context) {
    esp_err_t return_status = ESP_OK;
    MQTTStatus_t mqtt_status;
    MQTTFixedBuffer_t network_buffer;
    TransportInterface_t transport;
    memset(&transport, 0x00, sizeof(TransportInterface_t));

    assert( network_context != NULL );
    this->callback = callback;
    this->callback_context = callback_context;
    ESP_LOGI(TAG, "!!!!!!!! Callback is mn8 %p and this is %p", this->callback_context, this);

    // Fill in TransportInterface send and receive function pointers.
    // For this demo, TCP sockets are used to send and receive data
    // from network. Network context is SSL context for OpenSSL.
    transport.pNetworkContext = network_context;
    transport.send = espTlsTransportSend;
    transport.recv = espTlsTransportRecv;
    transport.writev = NULL;

    network_buffer.pBuffer = this->buffer;
    network_buffer.size = NETWORK_BUFFER_SIZE;

    mqtt_status = MQTT_Init( &this->mqtt_context,
                            &transport,
                            &MqttContext::sClock_get_time_ms,
                            &MqttContext::sEvent_callback,
                            &network_buffer );
    if( mqtt_status != MQTTSuccess )
    {
        return_status = ESP_FAIL;
        ESP_LOGE( TAG, "MQTT_Init failed: Status = %s.", MQTT_Status_strerror( mqtt_status ) );
        return return_status;
    }

    // mqtt_status = MQTT_InitStatefulQoS( &this->mqtt_context,
    //                                     outgoing_records,
    //                                     OUTGOING_PUBLISH_RECORD_LEN,
    //                                     incoming_records,
    //                                     INCOMING_PUBLISH_RECORD_LEN );

    // if( mqtt_status != MQTTSuccess )
    // {
    //     return_status = ESP_FAIL;
    //     ESP_LOGE( TAG, "MQTT_InitStatefulQoS failed: Status = %s.", MQTT_Status_strerror( mqtt_status ) );
    // }

    return return_status;
}

//******************************************************************************
uint32_t MqttContext::sClock_get_time_ms( void )
{
    /* esp_timer_get_time is in microseconds, converting to milliseconds */
    int64_t timeMs = esp_timer_get_time() / 1000;

    /* Libraries need only the lower 32 bits of the time in milliseconds, since
     * this function is used only for calculating the time difference.
     * Also, the possible overflows of this time value are handled by the
     * libraries. */
    return ( uint32_t ) timeMs;
}

//******************************************************************************
void MqttContext::event_callback(
    MQTTContext_t * mqtt_context,
    MQTTPacketInfo_t * packet_info,
    MQTTDeserializedInfo_t * deserialized_info
){
    assert( mqtt_context != NULL );
    assert( packet_info != NULL );
    assert( deserialized_info != NULL );

    ESP_LOGI(TAG, "!!!!!!!! Callback is %p with", this);

    if (this->callback != nullptr) {
        this->callback( mqtt_context, packet_info, deserialized_info, this->callback_context );
    }

    // if ((packet_info->type & 0xF0U) == MQTT_PACKET_TYPE_PUBLISH) {
    //     assert( deserialized_info->pPublishInfo != NULL );
    //     // subscription_handler->handle_publish( packet_info, deserialized_info );
    // } else {
    //     // pubsub_handler->handle_packet( packet_info, deserialized_info );
    // }
}