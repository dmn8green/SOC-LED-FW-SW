//*****************************************************************************
/**
 * @file cmd_provision_iot.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief REPL command to provision this device as an AWS iot thing
 * @version 0.1
 * @date 2023-09-04
 * 
 * @copyright Copyright (c) 2023
 */
//*****************************************************************************

#include "cmd_chargepoint.h"

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
#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)

extern MQTTContext_t *pmqttContext;

extern "C" int publishToTopic( MQTTContext_t * pMqttContext, char* topic, char* payload );

static struct {
    struct arg_str *command;
    struct arg_str *site_name;
    struct arg_str *station_id;
    struct arg_end *end;
} chargepoint_command_args;

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

    if (publishToTopic(pmqttContext, topic,  payload) != 0) {
        ESP_LOGE(TAG, "Failed to publish to topic");
        return -1;
    }
    ESP_LOGI(TAG, "Published to topic");
    return 0;
}

static int unprovision_charge_point_info(void) {
    return 0;
}

static int do_chargepoint_command(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&chargepoint_command_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, chargepoint_command_args.end, argv[0]);
        return 1;
    }

    const char* command = chargepoint_command_args.command->sval[0];
    const char* station_id = chargepoint_command_args.station_id->sval[0];
    const char* site_name = chargepoint_command_args.site_name->sval[0];

    if (command == nullptr || strlen(command) == 0) {
        command = "dump";
    }

    ESP_LOGI(TAG, "command %s", command);
    if (STR_IS_EQUAL(command, "dump")) {
        ESP_LOGI(TAG, "Dumping thing configuration");
        ChargePointConfig cpConfig;
        cpConfig.load();
        printf("    station_id: %s\n", cpConfig.get_station_id() ? cpConfig.get_station_id() : "<none>");
        printf("    site_name: %s\n", cpConfig.get_group_id() ? cpConfig.get_group_id() : "<none>");
        return 0;
    }

    if (STR_IS_EQUAL(command, "provision")) {
        printf("    new station_id: %s\n", station_id);
        printf("    new site_name: %s\n", site_name);
        return provision_charge_point_info(station_id, site_name);
    }

    if (STR_IS_EQUAL(command, "unprovision")) {
        ESP_LOGI(TAG, "Unprovisioning charge point info");
        return unprovision_charge_point_info();
    }

    ESP_LOGE(TAG, "Unknown command %s", command);
    return 1;
}

void register_chargepoint_command(void) {
    chargepoint_command_args.command = arg_str0(NULL, NULL, "<command>", "provision or unprovision");
    chargepoint_command_args.site_name = arg_str0(NULL, NULL, "<group or site name>", "Site name where this station is located");
    chargepoint_command_args.station_id = arg_str0(NULL, NULL, "<station id>", "Charge point station id");
    chargepoint_command_args.end = arg_end(0);

    const esp_console_cmd_t provision_charge_point_info_cmd = {
        .command = "chargepoint",
        .help = "chargepoint provision/unprovision",
        .hint = NULL,
        .func = &do_chargepoint_command,
        .argtable = &chargepoint_command_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&provision_charge_point_info_cmd));
}
