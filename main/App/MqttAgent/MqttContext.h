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

typedef void (* EventCallback_t )( struct MQTTContext * pContext,
                                   struct MQTTPacketInfo * pPacketInfo,
                                   struct MQTTDeserializedInfo * pDeserializedInfo,
                                   void* context );

class MqttContext : public NoCopy {
public:
    MqttContext(void);
    ~MqttContext(void) = default;

public:
    esp_err_t initialize(NetworkContext_t * network_context, EventCallback_t callback, void * pCallbackContext);
    inline MQTTContext_t * get_mqtt_context(void) { return &this->mqtt_context; }

private:
    typedef struct : MQTTContext_t {
        MqttContext *mn8_context = nullptr;
    } MN8MQTTContext_t;


    static uint32_t sClock_get_time_ms(void);

    static void sEvent_callback( MQTTContext_t * pMqttContext,
                                 MQTTPacketInfo_t * pPacketInfo,
                                 MQTTDeserializedInfo_t * pDeserializedInfo ) { 
                                    MN8MQTTContext_t* tmp = static_cast<MN8MQTTContext_t*>(pMqttContext);
                                    ESP_LOGI("MqttContext", "sEvent_callback %p mn8ctx = %p", tmp, tmp->mn8_context);
                                    tmp->mn8_context->event_callback(pMqttContext, pPacketInfo, pDeserializedInfo);
                                }

    void event_callback( MQTTContext_t * pMqttContext,
                         MQTTPacketInfo_t * pPacketInfo,
                         MQTTDeserializedInfo_t * pDeserializedInfo );

private:
    MQTTPubAckInfo_t outgoing_records[ OUTGOING_PUBLISH_RECORD_LEN ];
    MQTTPubAckInfo_t incoming_records[ INCOMING_PUBLISH_RECORD_LEN ];
    MN8MQTTContext_t mqtt_context;
    uint8_t * buffer = nullptr;
    EventCallback_t callback = nullptr;
    void * callback_context = nullptr;
};