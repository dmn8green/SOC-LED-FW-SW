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
#include "esp_log.h"

#include "esp_http_client.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"

#include "ArduinoJson.h"

#define MAX_HTTP_OUTPUT_BUFFER 8192
static const char *TAG = "provision_charge_point_info";

static struct {
    struct arg_str *site_name;
    struct arg_str *station_id;
    struct arg_end *end;
} provision_charge_point_info_args;


static int do_provision_charge_point_info(int argc, char **argv)
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

void cmd_provision_charge_point_info(void)
{
    static struct {
    struct arg_str *site_name;
    struct arg_str *station_id;
    struct arg_end *end;
} provision_charge_point_info_args;

    provision_charge_point_info_args.site_name = arg_str1(NULL, NULL, "<group or site name>", "Site name where this station is located");
    provision_charge_point_info_args.station_id = arg_str1(NULL, NULL, "<station id>", "Charge point station id");
    provision_iot_args.end = arg_end(2);

    const esp_console_cmd_t provision_charge_point_info_cmd = {
        .command = "provision_iot",
        .help = "provision device as an iot thing",
        .hint = NULL,
        .func = &do_provision_charge_point_info,
        .argtable = &provision_charge_point_info_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&provision_charge_point_info_cmd));
}
