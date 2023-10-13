#include "MqttConnection.h"
#include "MqttConfig.h"

#include "backoff_algorithm.h"
#include "esp_err.h"

#include <memory.h>

static const char* TAG = "MqttConnection";

//*****************************************************************************
esp_err_t MqttConnection::initialize(ThingConfig* thing_config) {
    memset( &this->network_context, 0x00, sizeof( NetworkContext_t ) );

    this->thing_config = thing_config;

    this->network_context.pcHostname = this->thing_config->get_endpoint_address();
    this->network_context.xPort = AWS_MQTT_PORT;
    this->network_context.pxTls = NULL;
    this->network_context.xTlsContextSemaphore = xSemaphoreCreateMutexStatic(&xTlsContextSemaphoreBuffer);

    this->network_context.pcServerRootCA = this->thing_config->get_root_ca();
    this->network_context.pcServerRootCASize = strlen(this->network_context.pcServerRootCA) + 1;

    this->network_context.pcClientCert = this->thing_config->get_certificate_pem();
    this->network_context.pcClientCertSize = strlen(this->network_context.pcClientCert)+1;
    this->network_context.pcClientKey = this->thing_config->get_private_key();
    this->network_context.pcClientKeySize = strlen(this->network_context.pcClientKey)+1;

    return ESP_OK;
}

//*****************************************************************************
esp_err_t MqttConnection::connect_with_retries(MqttContext* mqtt_context, uint16_t retry_count) {
    esp_err_t ret = ESP_OK;
    BackoffAlgorithmStatus_t backoff_alg_status = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t reconnect_params;
    uint16_t next_retry_back_off;

    BackoffAlgorithm_InitializeParams( &reconnect_params,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       retry_count );

    do {
        ret = xTlsConnect ( &this->network_context ) == TLS_TRANSPORT_SUCCESS ? ESP_OK : ESP_FAIL;
        if (ret == ESP_OK) {
            ESP_LOGI( TAG, "TLS connection established." );
            ret = connect_mqtt_socket( mqtt_context );
            if (ret != ESP_OK) {
                xTlsDisconnect( &this->network_context );
            } else {
                ESP_LOGI( TAG, "MQTT connection established." );
            }
        }

        ESP_LOGI( TAG, "MQTT connection attempt %ld of %ld.",
                  reconnect_params.attemptsDone,
                  reconnect_params.maxRetryAttempts);

        ESP_LOGI( TAG, "ret is %d and backoff_alg_status is %d", ret, backoff_alg_status);

        if (ret != ESP_OK) {
            backoff_alg_status = BackoffAlgorithm_GetNextBackoff( &reconnect_params, rand(), &next_retry_back_off );
            if ( backoff_alg_status == BackoffAlgorithmRetriesExhausted ) {
                ESP_LOGE( TAG, "Connection retry attempts exhausted." );
                break;
            } else if ( backoff_alg_status == BackoffAlgorithmSuccess ) {
                ESP_LOGE( TAG, "Connection to broker failed.  Retrying connection after %hu ms backoff.", next_retry_back_off );
                vTaskDelay( next_retry_back_off/portTICK_PERIOD_MS );
            }
        }
    } while( ret != ESP_OK && backoff_alg_status == BackoffAlgorithmSuccess );

    // If we succeeded, remember it, this is use later when we reconnect.
    this->client_session_present = ret == ESP_OK;

    return ret;
}

//*****************************************************************************
esp_err_t MqttConnection::disconnect(MqttContext* mqtt_context) {
    esp_err_t ret = ESP_OK;
    MQTTStatus_t mqtt_status;

    assert( mqtt_context != NULL );

    // Send MQTT DISCONNECT packet to broker.
    mqtt_status = MQTT_Disconnect( mqtt_context );

    if( mqtt_status != MQTTSuccess ) {
        ret = ESP_FAIL;
        ESP_LOGE( TAG, "Failed to disconnect MQTT connection with broker with error = %s.",
                  MQTT_Status_strerror( mqtt_status ) );
    } else {
        ESP_LOGI( TAG, "Successfully disconnected MQTT connection with broker." );
    }

    // Disconnect TLS session.
    if( xTlsDisconnect( &this->network_context ) != TLS_TRANSPORT_SUCCESS ) {
        ret = ESP_FAIL;
        ESP_LOGE( TAG, "Failed to disconnect TLS session." );
    } else {
        ESP_LOGI( TAG, "Successfully disconnected TLS session." );
    }

    return ret;
}

//*****************************************************************************
esp_err_t MqttConnection::connect_mqtt_socket(MqttContext* mqtt_context) {
    esp_err_t ret = ESP_OK;
    MQTTStatus_t mqtt_status;
    MQTTConnectInfo_t connect_info;
    memset( &connect_info, 0x00, sizeof( MQTTConnectInfo_t ) );

    assert( mqtt_context != NULL );

    /* Establish MQTT session by sending a CONNECT packet. */

    // If #createCleanSession is true, start with a clean session i.e. direct
    // the MQTT broker to discard any previous session data.
    // If #createCleanSession is false, directs the broker to attempt to
    // reestablish a session which was already present.
    connect_info.cleanSession = !client_session_present;

    // The client identifier is used to uniquely identify this MQTT client to
    // the MQTT broker. In a production device the identifier can be something
    // unique, such as a device serial number.
    connect_info.pClientIdentifier = this->thing_config->get_thing_name();
    connect_info.clientIdentifierLength = strlen(connect_info.pClientIdentifier);

    // The maximum time interval in seconds which is allowed to elapse between
    // two Control Packets.  It is the responsibility of the Client to ensure
    // that the interval betwee Control Packets being sent does not exceed the
    // this Keep Alive value. In the absence of sending any other Control
    // Packets, the Client MUST send a PINGREQ Packet. 
    connect_info.keepAliveSeconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS;

    // Password for authentication is not used with aws.
    connect_info.pUserName = METRICS_STRING;
    connect_info.userNameLength = METRICS_STRING_LENGTH;
    connect_info.pPassword = NULL;
    connect_info.passwordLength = 0U;

    // Send MQTT CONNECT packet to broker.
    mqtt_status = MQTT_Connect( 
        mqtt_context, &connect_info, NULL, 
        CONNACK_RECV_TIMEOUT_MS, &this->broker_session_present
    );

    if( mqtt_status != MQTTSuccess ) {
        ret = ESP_FAIL;
        ESP_LOGE( TAG, "Connection with MQTT broker failed with status %s.",
                  MQTT_Status_strerror( mqtt_status ) );
    } else {
        ESP_LOGI( TAG, "MQTT connection successfully established with broker.\n\n" );
    }

    return ret;
}