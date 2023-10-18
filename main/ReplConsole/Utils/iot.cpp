//*****************************************************************************
/**
 * @file iot.c
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief iot helper, methods to provision/unprovision a device, implementation
 * @version 0.1
 * @date 2023-10-18
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//*****************************************************************************

#include "iot.h"

#include "Utils/FuseMacAddress.h"
#include "App/Configuration/ThingConfig.h"

#include "ArduinoJson.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_system.h"
#include "esp_tls.h"

#include "mbedtls/base64.h"

#include <sys/param.h>


//*****************************************************************************
// Local defines

/**
 * @brief Size of buffer to receive http response payload
 * 
 * The provisioning service returns a json payload with the following format:
 * {
 *      "thingId": "string",
 *      "thingName": "string",
 *      "thingArn": "string",
 *      "thingCertArn": "string",
 *      "thingCertId": "string",
 *      "thingCertPem": "string",
 *      "thingPrivateKey": "string",
 *      "thingRootCa": "string",
 *      "iotEndpoint": "string"
 * }
 * 
 * The Certs and private key are fairly big.  The biggest one is the 
 * thingCertPem which is around 2,048 bytes.  The other ones are around 1,024
 * bytes. The total size of the payload is around 7,000 bytes.  We will use
 * a buffer of 8,192 bytes to be safe.
 */
#define MAX_HTTP_OUTPUT_BUFFER 8192
#define PROVISION_URL "https://ir5q3elml3.execute-api.us-east-1.amazonaws.com/dev/provision"
#define UNPROVISION_URL "https://ir5q3elml3.execute-api.us-east-1.amazonaws.com/dev/unprovision"
#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)
#define STR_IS_EMPTY(str) (strlen(str) == 0)

static const char * TAG = "iot";

//*****************************************************************************
// Local function prototypes

static int make_http_call(const char* url, const char* username, const char* password, char* response_buffer);
static bool register_thing_from_http_response(const char* response_buffer);

//*****************************************************************************
// Public functions implementation

//*****************************************************************************
int provision_device(const char* url, const char* username, const char* password) {
    ESP_LOGI(TAG, "url %s", url);
    ESP_LOGI(TAG, "username %s", username);
    ESP_LOGI(TAG, "password %s", password);

    char *response_buffer = (char*) malloc(MAX_HTTP_OUTPUT_BUFFER + 1);
    memset(response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER + 1);

    int ret = make_http_call(
        STR_IS_EMPTY(url) ? PROVISION_URL : url, 
        username, password,
        response_buffer
    );
    if (ret == 0) {
        // provisioned successfully, save the config
        if (!register_thing_from_http_response(response_buffer)) {
            ESP_LOGE(TAG, "Failed to process api response");
            ret = 1;
        }
    }

    free (response_buffer);
    return ret;
}

//*****************************************************************************
int unprovision_device(const char* url, const char* username, const char* password) {
    ESP_LOGI(TAG, "url %s", url);
    ESP_LOGI(TAG, "username %s", username);
    ESP_LOGI(TAG, "password %s", password);

    char *response_buffer = (char*) malloc(MAX_HTTP_OUTPUT_BUFFER + 1);
    memset(response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER + 1);

    int ret = make_http_call(
        STR_IS_EMPTY(url) ? UNPROVISION_URL : url, 
        username, password,
        response_buffer
    );
    if (ret == 0) {
        ThingConfig thingConfig;
        thingConfig.reset();
    }

    free (response_buffer);
    return ret;
}

//*****************************************************************************
// Local functions implementation

//*****************************************************************************
/**
 * @brief Process the response from the provisioning service
 * 
 * @param response_buffer 
 * @return true 
 * @return false 
 */
static bool register_thing_from_http_response(const char* response_buffer)
{
    DynamicJsonDocument doc(MAX_HTTP_OUTPUT_BUFFER + 1);
    DeserializationError error = deserializeJson(doc, response_buffer);
    if (error) {
        ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
        return false;
    }

    JsonObject root = doc.as<JsonObject>();
    auto thing = root["thing"];
    if (thing.isNull()) {
        ESP_LOGE(TAG, "thing is null");
        return false;
    }
    
    auto certs = root["certs"];
    if (certs.isNull()) {
        ESP_LOGE(TAG, "certs is null");
        return false;
    }
    
    auto endpoint = root["endpoint"];
    if (endpoint.isNull()) {
        ESP_LOGE(TAG, "endpoint is null");
        return false;
    }


    // Populate the IOT configuration in flash.
    ThingConfig thingConfig;
    
    thingConfig.set_thing_name(thing["thingName"]);
    thingConfig.set_certificate_arn(certs["certificateArn"]);
    thingConfig.set_certificate_id(certs["certificateId"]);
    thingConfig.set_certificate_pem(certs["certificatePem"]);
    thingConfig.set_private_key(certs["keyPair"]["PrivateKey"]);
    thingConfig.set_public_key(certs["keyPair"]["PublicKey"]);
    thingConfig.set_endpoint_address(endpoint["endpointAddress"]);
    
    thingConfig.save();

    return true;
}

//*****************************************************************************
/**
 * @brief Event handler for http client
 * 
 * Function lifted from 
 * esp-idf/examples/protocols/https_request/main/https_request_example.c
 * 
 * @param evt 
 * @return esp_err_t 
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    esp_err_t err;
    int mbedtls_err = 0;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy((void *)((char *)evt->user_data + output_len), evt->data, copy_len);
                    }
                } else {
                    const int buffer_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(buffer_len);
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (buffer_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}


//*****************************************************************************
/**
 * @brief 
 * 
 * This code is lifted from the esp-idf example https_request
 * 
 * @param url 
 * @param username 
 * @param password 
 * @return int 
 */
int make_http_call(
    const char* url, 
    const char* username, 
    const char* password,
    char* response_buffer
) {
    int ret = 0;
    
    #pragma GCC diagnostic pop "-Wmissing-field-initializers" 
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .disable_auto_redirect = true,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    #pragma GCC diagnostic push "-Wmissing-field-initializers" 

    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_raw[64] = {0};
    snprintf(auth_raw, 64, "%s:%s", username, password);

    size_t outlen;
    char auth_base64[64] = {0};
    strcpy(auth_base64, "Basic ");
    char* base64_ptr = auth_base64+strlen("Basic ");
    mbedtls_base64_encode((unsigned char*) base64_ptr, 64, &outlen, (unsigned char*) auth_raw, strlen(auth_raw));
    ESP_LOGI(TAG, "auth_base64 %s", auth_base64);

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);
    char post_data[32] = {0};
    snprintf((char*) post_data, 32, "{\"thingId\":\"%s\"}", mac_address);
    ESP_LOGI(TAG, "post_data %s len %d", post_data, strlen(post_data));

    esp_http_client_set_header(client, "Authorization", auth_base64);
    esp_http_client_set_header(client, "Content-Type", "application/json; charset=utf-8");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK && esp_http_client_get_status_code(client) == 200) {
        ESP_LOGI(TAG, "HTTPS Status = %s", response_buffer);
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
        ret = 1;
    }

    esp_http_client_cleanup(client);
    return ret;
}
