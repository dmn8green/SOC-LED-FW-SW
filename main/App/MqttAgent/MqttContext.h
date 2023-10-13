#pragma once

#include "Utils/NoCopy.h"

#include "core_mqtt.h"
#include "core_mqtt_state.h"

#include "esp_err.h"

/**
 * @brief The length of the outgoing publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for outgoing publishes.
 */
#define OUTGOING_PUBLISH_RECORD_LEN    ( 10U )

/**
 * @brief The length of the incoming publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for incoming publishes.
 */
#define INCOMING_PUBLISH_RECORD_LEN    ( 10U )

#define NETWORK_BUFFER_SIZE     ( 1024U )
#define OS_NAME                 "FreeRTOS"
#define OS_VERSION              tskKERNEL_VERSION_NUMBER
#define HARDWARE_PLATFORM_NAME  "ESP32"
#define AWS_MQTT_PORT           ( 8883 )

#include "core_mqtt.h"
#define MQTT_LIB    "core-mqtt@" MQTT_LIBRARY_VERSION

typedef void (* EventCallback_t )( struct MQTTContext * pContext,
                                   struct MQTTPacketInfo * pPacketInfo,
                                   struct MQTTDeserializedInfo * pDeserializedInfo,
                                   void* context );

class MqttContext : public NoCopy, public MQTTContext_t {
public:
    MqttContext(void);
    ~MqttContext(void) = default;

public:
    esp_err_t initialize(NetworkContext_t * network_context, EventCallback_t callback, void * pCallbackContext);

private:
    static uint32_t sClock_get_time_ms(void);

    static void sEvent_callback( MQTTContext_t * pMqttContext,
                                 MQTTPacketInfo_t * pPacketInfo,
                                 MQTTDeserializedInfo_t * pDeserializedInfo ) { 
                                    ((MqttContext*)pMqttContext)->event_callback(pMqttContext, pPacketInfo, pDeserializedInfo);
                                }

    void event_callback( MQTTContext_t * pMqttContext,
                         MQTTPacketInfo_t * pPacketInfo,
                         MQTTDeserializedInfo_t * pDeserializedInfo );

private:
    MQTTPubAckInfo_t outgoing_records[ OUTGOING_PUBLISH_RECORD_LEN ];
    MQTTPubAckInfo_t incoming_records[ INCOMING_PUBLISH_RECORD_LEN ];
    uint8_t * buffer = nullptr;
    EventCallback_t callback = nullptr;
    void * pCallbackContext = nullptr;
};