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
#include "App/MN8App.h"

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
#define STR_IS_NULL_OR_EMPTY(str) (str == nullptr || strlen(str) == 0)

extern MQTTContext_t *pmqttContext;

static struct
{
    struct arg_str *command;
    struct arg_str *group_name;
    struct arg_str *station_id_1;
    struct arg_int *port_number_1;
    struct arg_str *station_id_2;
    struct arg_int *port_number_2;
    struct arg_end *end;
} chargepoint_command_args;

char payload[1024] = {0};
char topic[64] = {0};

//*****************************************************************************
/**
 * @brief Sends the mqtt message to reset the proxy
 * 
 * When provisioning/unprovisioning the charge point info, we need to inform
 * the associated proxy that the data was changed.  This is done by sending
 * a message to the topic group_name/refresh where group_name is the name of
 * the group where this station is located.
 * 
 * The proxy will then reload the data from the database.
 * 
 * @param group_name 
 * @return int 
 */
static int publish_reset_proxy_message(const char* group_name) {
    esp_err_t ret = ESP_OK;
    auto &mn8_app = MN8App::instance();

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *)topic, 64, "%s/refresh", group_name);
    snprintf((char *) payload, sizeof(payload), "{}");

    ESP_LOGI(TAG, "Inform the associated proxy that the data was changed: %s", topic);
    ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to refresh the proxy %s, it may need to be manually restarted", group_name);
    }
    return ret == ESP_OK ? 0 : 1;
}

static int publish_get_latest_message(void) {
    esp_err_t ret = ESP_OK;
    auto &mn8_app = MN8App::instance();

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *) topic, 64, "%s/latest", mac_address);
    snprintf((char *) payload, sizeof(payload), "{}");

    ESP_LOGI(TAG, "Request latest from proxy: %s", topic);
    ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to request latest for our thing %s", mac_address);
    }
    return ret == ESP_OK ? 0 : 1;
}

//*****************************************************************************
/**
 * @brief Persists the charge point info to the NVS
 * 
 * This function persists the charge point info to the NVS.  It is assumed that
 * the info is valid and that the caller has validated the info.
 * 
 * @param group_name     Name of the group where this station is located
 * @param station_id_1  Charge point station id associated with led 1
 * @param port_number_1 Charge point station charger port number associated with led 1
 * @param station_id_2  Charge point station id associated with led 2
 * @param port_number_2 Charge point station charger port number associated with led 2
 * 
 * @return int        0 if successful, 1 otherwise
 */
static int persist_charge_point_info(
    const char *group_name, 
    const char *station_id_1, uint8_t port_number_1,
    const char *station_id_2, uint8_t port_number_2
) {

    ESP_LOGI(TAG, "Provisioning charge point info");

    ChargePointConfig cpConfig;
    cpConfig.set_chargepoint_info(
        group_name,
        station_id_1, port_number_1,
        station_id_2, port_number_2
    );

    if (cpConfig.save() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save charge point config, something is wrong with your board.");
        return 1;
    }

    return 0;
}

//*****************************************************************************
/**
 * @brief Sends the mqtt message to provision the charge point information
 * 
 * This function sends the mqtt message to provision the charge point
 * information.  The message is sent to the topic group_id/register_station where
 * group_id is the group name where this station is located (or any other grouping)
 * 
 * @param group_name    Name of the group where this station is located
 * @param station_id_1  Charge point station id associated with led 1
 * @param port_number_1 Charge point station charger port number associated with led 1
 * @param station_id_2  Charge point station id associated with led 2
 * @param port_number_2 Charge point station charger port number associated with led 2
 * 
 * @return int          0 if successful, 1 otherwise
 */
static int provision_charge_point_info(
    const char *group_name, 
    const char *station_id_1, uint8_t port_number_1,
    const char *station_id_2, uint8_t port_number_2
) {
    auto &mn8_app = MN8App::instance();

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *)topic, 64, "%s/register_station", group_name);
    snprintf(
        payload, sizeof(payload),
        R"(
            {
                "thing_id":"%s",
                "group_id":"%s",
                "leds": [{
                    "port": %d,
                    "station": "%s",
                    "led": 0,
                    "last_state": "unknown",
                    "last_charge": 0
                },{
                    "port": %d,
                    "station": "%s",
                    "led": 1,
                    "last_state": "unknown",
                    "last_charge": 0
                }]
            }
        )",
        mac_address, group_name,
        port_number_1, station_id_1,
        port_number_2, station_id_2
    );

    ESP_LOGD(TAG, "Send the register message to topic: %s", topic);
    printf("Mapping leds with chargers...\n");
    esp_err_t ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    if (ret == ESP_ERR_TIMEOUT) {
        printf("Timeout trying to pair led with station port\n");
        printf("Try again: Hit the up arrow and hit enter\n");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to pair led with charger%s\nMake sure you have a valid network connection", topic);
        return 1;
    } else {
        printf("Waiting for the mapping to have been completed...\n");
        // Give sometimes for the lambda to run to ensure the mapping is in the
        // database by the time we ask the proxy to refresh
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    printf("next type: chargepoint refresh\n");
    printf("followed by: chargepoint latest\n");

    // This is somewhat unreliable.  We should have a way to know when the
    // proxy has refreshed the data.
    // printf("Asking proxy broker to refresh...\n");
    // ret = publish_reset_proxy_message(group_name);
    // if (ret == ESP_ERR_TIMEOUT) {
    //     printf("Timeout waiting for proxy to refresh\n");
    //     printf("Try the following command: chargepoint refresh\n");
    //     printf("Followed by the following command: chargepoint latest\n");
    // } else if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to refresh the proxy %s, it may need to be manually restarted", group_name);
    //     return 1;
    // } else {
    //     printf("Waiting for proxy to have refreshed...\n");
    //     vTaskDelay(5000 / portTICK_PERIOD_MS);
    // }

    // printf("Requesting latest....\n");
    // memset(topic, 0, sizeof(topic));
    // memset(payload, 0, sizeof(payload));
    // snprintf((char *) topic, 64, "%s/latest", mac_address);
    // snprintf((char *) payload, sizeof(payload), "{}");
    // ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    // if (ret == ESP_ERR_TIMEOUT) {
    //     printf("Timeout waiting for proxy to refresh\n");
    //     printf("Try the following command: chargepoint latest\n");
    // } else if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to request latest for our thing %s", mac_address);
    //     return 1;
    // }

    return 0;
}

//*****************************************************************************
/**
 * @brief Sends the mqtt message to unprovision the charge point information
 * 
 * This assume that the charge point info is already persisted in the NVS.
 * If it isn't then this function will fail.
 * 
 * This unprovision both LEDs associated with this thing.
 * 
 * We try to clean as much as we can with what info we have.  In case this
 * device was wiped clean before unprovisioning, we still try to send the
 * mqtt message to get the element removed from the database.  But we will have
 * no way to tell the proxy.
 * 
 * @return int 
 */
static int unprovision_charge_point_info()
{
    ChargePointConfig cpConfig;
    cpConfig.load();

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);
    snprintf((char *)topic, 64, "%s/unregister_station", cpConfig.is_configured() ? cpConfig.get_group_id(): "unknown");
    snprintf(payload, sizeof(payload), 
        R"(
            {
                "thing_id":"%s"
            }
        )",
        mac_address
    );

    ESP_LOGI(TAG, "Send the unregister message to topic: %s", topic);
    auto &mn8_app = MN8App::instance();

    // Unprovision the charge point info
    esp_err_t ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unpair led with charger%s\nMake sure you have a valid network connection", topic);
        return 1;
    }

    if (cpConfig.is_configured()) {
        printf("Disassociating leds with chargers...\n");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        if (publish_reset_proxy_message(cpConfig.get_group_id()) != 0) {
            ESP_LOGE(TAG, "Failed to refresh the proxy %s, it may need to be manually restarted", cpConfig.get_group_id());
            return 1;
        }
        return cpConfig.reset();
    }
    return 0;
}

//*****************************************************************************
/**
 * @brief REPL command to pair this device leds with a charge point station port
 * 
 * @param argc   Number of arguments
 * @param argv   Array of arguments
 * @return int   0 if successful, 1 otherwise
 */
static int do_chargepoint_command(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&chargepoint_command_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, chargepoint_command_args.end, argv[0]);
        return 1;
    }

    const char *command         = chargepoint_command_args.command->sval[0];
    const char *group_name       = chargepoint_command_args.group_name->sval[0];
    const char *station_id_1    = chargepoint_command_args.station_id_1->sval[0];
    const uint8_t port_number_1 = chargepoint_command_args.port_number_1->ival[0];
    const char *station_id_2    = chargepoint_command_args.station_id_2->sval[0];
    const uint8_t port_number_2 = chargepoint_command_args.port_number_2->ival[0];

    printf("command: %s\n", command);
    printf("group_name: %s\n", group_name);
    printf("station_id_1: %s\n", station_id_1);
    printf("port_number_1: %d\n", port_number_1);
    printf("station_id_2: %s\n", station_id_2);
    printf("port_number_2: %d\n", port_number_2);

    ESP_LOGI(TAG, "executing command %s", command);
    if (STR_IS_EQUAL(command, "dump")) {
        ESP_LOGI(TAG, "Dumping thing configuration");
        ChargePointConfig cpConfig;
        if (cpConfig.load() == ESP_OK) {
            cpConfig.dump();
        } else {
            printf("No charge point info found for this mn8 thing\n");
        }
        return 0;
    }

    if (STR_IS_EQUAL(command, "refresh")) {
        ESP_LOGI(TAG, "Refresh associated proxy");
        ChargePointConfig cpConfig;
        if (cpConfig.load() == ESP_OK) {
            publish_reset_proxy_message(cpConfig.get_group_id());
        } else {
            printf("No charge point info found for this mn8 thing\n");
        }
        return 0;
    }

    if (STR_IS_EQUAL(command, "latest")) {
        ESP_LOGI(TAG, "Requesting latest charge point info");
        ChargePointConfig cpConfig;
        if (cpConfig.load() == ESP_OK) {
            publish_get_latest_message();
        } else {
            printf("No charge point info found for this mn8 thing\n");
        }
        return 0;
    }

    if (STR_IS_EQUAL(command, "provision")) {
        // validate args
        if (STR_IS_NULL_OR_EMPTY(group_name)) { ESP_LOGE(TAG, "Missing group name"); return 1; }
        if (STR_IS_NULL_OR_EMPTY(station_id_1)) { ESP_LOGE(TAG, "Missing station id 1"); return 1; }
        if (port_number_1 != 1 && port_number_1 != 2) { ESP_LOGE(TAG, "Invalid port number 1"); return 1; }
        if (STR_IS_NULL_OR_EMPTY(station_id_2)) { ESP_LOGE(TAG, "Missing station id 2"); return 1; }
        if (port_number_2 != 1 && port_number_2 != 2) { ESP_LOGE(TAG, "Invalid port number 2"); return 1; }

        printf("Provisioning charge point info\n");
        printf("    group_name: %s\n", group_name);
        printf("    station_id_1: %s\n", station_id_1);
        printf("    port_number_1: %d\n", port_number_1);
        printf("    station_id_2: %s\n", station_id_2);
        printf("    port_number_2: %d\n", port_number_2);

        // First persists the info to the NVS
        if (persist_charge_point_info(group_name, station_id_1, port_number_1, station_id_2, port_number_2) != 0) {
            ESP_LOGE(TAG, "Failed to persist charge point info");
            return 1;
        }

        // Then send the mqtt message to provision the charge point info.
        // The recipient of the message is the charge point proxy and will
        // take care of updating the database.
        return provision_charge_point_info(group_name, station_id_1, port_number_1, station_id_2, port_number_2);
    }

    if (STR_IS_EQUAL(command, "unprovision")) {
        ESP_LOGI(TAG, "Unprovisioning charge point info");
        return unprovision_charge_point_info();
    }

    ESP_LOGE(TAG, "Unknown command %s", command);
    return 1;
}

//*****************************************************************************
/**
 * @brief Register the chargepoint command with the REPL
 * 
 * @return void
 */
void register_chargepoint_command(void)
{
    
    chargepoint_command_args.command       = arg_str1(NULL, NULL, "<dump/provision/unprovision/refresh/latest>", "print current info, assign_led, provision or unprovision, refresh proxy, force proxy to send us latest");
    chargepoint_command_args.group_name    = arg_str0(NULL, NULL, "<group or group name>", "Site name where this station is located");
    chargepoint_command_args.station_id_1  = arg_str0(NULL, NULL, "<station id>", "Charge point station id associated with led 1");
    chargepoint_command_args.port_number_1 = arg_int0(NULL, NULL, "<1 or 2>", "Charger port number associated with led 1");
    chargepoint_command_args.station_id_2  = arg_str0(NULL, NULL, "<station id>", "Charge point station id associated with led 2");
    chargepoint_command_args.port_number_2 = arg_int0(NULL, NULL, "<1 or 2>", "Charger port number associated with led 2");

    chargepoint_command_args.end = arg_end(5);

    const esp_console_cmd_t provision_charge_point_info_cmd = {
        .command = "chargepoint",
        .help = "chargepoint provision/unprovision",
        .hint = NULL,
        .func = &do_chargepoint_command,
        .argtable = &chargepoint_command_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&provision_charge_point_info_cmd));
}
