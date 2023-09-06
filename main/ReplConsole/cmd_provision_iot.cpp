//*****************************************************************************
/**
 * @file cmd_provision_iot.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief REPL command to provision this device as an AWS iot thing
 * @version 0.1
 * @date 2023-09-04
 * 
 * @copyright Copyright (c) 2023
 * 
 * The code from this file was adapted from the 
 * esp-idf/examples/protocols/https_request/main/https_request_example.c
 * 
 * The original code is licensed as follows:
 *   BEGIN LICENSE BLOCK
 *   This example code is in the Public Domain (or CC0 licensed, at your option.)
 *   
 *   Unless required by applicable law or agreed to in writing, this
 *   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *   CONDITIONS OF ANY KIND, either express or implied.
 *   END LICENSE BLOCK
 */

#include "cmd_provision_iot.h"

#include "Utils/FuseMacAddress.h"
#include "IOT/ThingConfig.h"

#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mbedtls/base64.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "esp_log.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"

#include "ArduinoJson.h"

#define MAX_HTTP_OUTPUT_BUFFER 8192
static const char *TAG = "provision_iot";

static struct {
    struct arg_str *url;
    struct arg_str *username;
    struct arg_str *password;
    struct arg_end *end;
} provision_iot_args;

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
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
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
 * @brief Process the response from the provisioning server
 * 
 * @param response_buffer   JSON string response from the provisioning server
 * @return true             The response was successfully processed
 * @return false            The response could not be processed
 */
static bool process_api_response(const char* response_buffer)
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
 * @brief Provision this device as an AWS iot thing
 * 
 * This function is registered as a REPL command. 
 * It takes the following arguments:
 *  - url: url to the provisioning server
 *  - username: username for the provisioning server
 *  - password: password for the provisioning server
 * 
 * The function will send a POST request to the provisioning server with the
 * following JSON payload:
 * {
 *  "thingId": "mac_address of this device"
 * }
 * 
 * The mac address is obtained from the esp-idf API esp_efuse_mac_get_default()
 * This is the same mac address returned when calling esptool.py chip_id
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
static int do_provision_iot_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&provision_iot_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, provision_iot_args.end, argv[0]);
        return 1;
    }

    char *local_response_buffer = (char*) malloc(MAX_HTTP_OUTPUT_BUFFER + 1);
    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER + 1);

    #pragma GCC diagnostic pop "-Wmissing-field-initializers" 
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    esp_http_client_config_t config = {
        .url = provision_iot_args.url->sval[0],
        .method = HTTP_METHOD_POST,
        .disable_auto_redirect = true,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    #pragma GCC diagnostic push "-Wmissing-field-initializers" 

    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_raw[64] = {0};
    snprintf(auth_raw, 64, "%s:%s", provision_iot_args.username->sval[0], provision_iot_args.password->sval[0]);

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
        // ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %" PRIu64,
        //         esp_http_client_get_status_code(client),
        //         esp_http_client_get_content_length(client));
        
        ESP_LOGI(TAG, "HTTPS Status = %s", local_response_buffer);
        return process_api_response(local_response_buffer);
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return 0;
}

//*****************************************************************************
/**
 * @brief Register the provision_iot REPL command
 * 
 * This REPL command is only available if the device hasn't been provisioned
 * yet in the AWS IOT cloud.  Once provisioned, the device will have a
 * certificate and private key that will be used to authenticate with the
 * AWS IOT cloud.
 * 
 * This command shall only be used by users who have access to the provisioning
 * server. (AppliedLogix employees)  Eventually, this will be updated for
 * factory production, either via an external PC script, or something else.
 * 
 * This is a temporary solution.
 */
void register_provision_iot(void)
{
    provision_iot_args.url = arg_str1(NULL, NULL, "<url>", "Url to provisionign server");
    provision_iot_args.username = arg_str1(NULL, NULL, "<username>", "API username");
    provision_iot_args.password = arg_str1(NULL, NULL, "<password>", "API password");
    provision_iot_args.end = arg_end(4);

    const esp_console_cmd_t provision_iot_cmd = {
        .command = "provision_iot",
        .help = "provision device as an iot thing",
        .hint = NULL,
        .func = &do_provision_iot_cmd,
        .argtable = &provision_iot_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&provision_iot_cmd));
}
