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
//*****************************************************************************

#include "cmd_provision_iot.h"

#include "Utils/FuseMacAddress.h"
#include "App/Configuration/ChargePointConfig.h"

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

#include "esp_console.h"
#include "argtable3/argtable3.h"

#include "ArduinoJson.h"

/* MQTT API headers. */
// THis needs to change to use the mqtt queue directly
#include "core_mqtt.h"
#include "core_mqtt_state.h"

#define MAX_HTTP_OUTPUT_BUFFER 8192
static const char *TAG = "provision_charge_point_info";

extern MQTTContext_t *pmqttContext;

extern "C" int publishToTopic( MQTTContext_t * pMqttContext, char* topic, char* payload );

static struct {
    struct arg_str *site_name;
    struct arg_str *station_id;
    struct arg_end *end;
} provision_charge_point_info_args;

char payload[256] = {0};

//*****************************************************************************
/**
 * @brief Pair this iot device with the chargepoint station
 */
static int provision_charge_point_info(const char* station_id, const char* site_name) {
    ChargePointConfig cpConfig;

    cpConfig.set_group_id(site_name);
    cpConfig.set_station_id(station_id);
    if (cpConfig.save() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save charge point config");
        return 1;
    }

    // TODO This should ask the app to register the station.
    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);
    char topic[64] = {0};
    snprintf((char*) topic, 64, "%s/register_station", mac_address);

    snprintf(payload, 256, "{\"stationId\":\"%s\",\"groupId\":\"%s\",\"description\":\"%s\"}", 
        station_id, site_name, ""
    );

    ESP_LOGI(TAG, "Publishing to topic: %s", topic);

    // if (publishToTopic(pmqttContext, topic,  payload) != 0) {
    //     ESP_LOGE(TAG, "Failed to publish to topic");
    //     return -1;
    // }
    ESP_LOGI(TAG, "Published to topic");
    return 0;
}

//*****************************************************************************
/**
 * @brief Pair this iot device with the chargepoint station
 * 
 * This will publish a mqtt message to register the station.  The info is then
 * stored in a database and paired with this esp32 board.
 */
static int do_provision_charge_point_info(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&provision_charge_point_info_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, provision_charge_point_info_args.end, argv[0]);
        return 1;
    }

    const char* station_id = provision_charge_point_info_args.station_id->sval[0];
    const char* site_name = provision_charge_point_info_args.site_name->sval[0];

    if (station_id == NULL || strlen(station_id) == 0 || site_name == NULL || strlen(site_name) == 0) {
        ESP_LOGI(TAG, "station_id or site_name not specified");
        ESP_LOGI(TAG, "printing current info");

        ChargePointConfig cpConfig;
        cpConfig.load();
        printf("    station_id: %s\n", cpConfig.get_station_id() ? cpConfig.get_station_id() : "<none>");
        printf("    site_name: %s\n", cpConfig.get_group_id() ? cpConfig.get_group_id() : "<none>");
        return 0;
    } else  {
        ESP_LOGI(TAG, "    new station_id: %s", station_id);
        ESP_LOGI(TAG, "    new site_name: %s", site_name);
        return provision_charge_point_info(station_id, site_name);
    }
}

void register_provision_charge_point_info(void) {
    provision_charge_point_info_args.site_name = arg_str1(NULL, NULL, "<group or site name>", "Site name where this station is located");
    provision_charge_point_info_args.station_id = arg_str1(NULL, NULL, "<station id>", "Charge point station id");
    provision_charge_point_info_args.end = arg_end(0);

    const esp_console_cmd_t provision_charge_point_info_cmd = {
        .command = "provision_cp",
        .help = "provision cp info",
        .hint = NULL,
        .func = &do_provision_charge_point_info,
        .argtable = &provision_charge_point_info_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&provision_charge_point_info_cmd));
}
