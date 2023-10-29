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

extern MQTTContext_t *pmqttContext;

static struct
{
    struct arg_str *command;
    struct arg_str *site_name;
    struct arg_int *led_number;
    struct arg_str *station_id;
    struct arg_int *port_number;
    struct arg_end *end;
} chargepoint_command_args;

char payload[256] = {0};
char topic[64] = {0};

//*****************************************************************************
/**
 * @brief Persists the charge point info to the NVS
 * 
 * Each station charge port is associated with a led.  The led number is used
 * to specify which led to associate with the charger.
 *
 * @param site_name   Name of the site where this station is located
 * @param led_number  Led number to associate with the charger
 * @param station_id  Charge point station id
 * @param port_number Charge point station charger port number
 * 
 * @return int        0 if successful, 1 otherwise
 */
static int persist_charge_point_info(const char *site_name, uint8_t led_number, const char *station_id, uint8_t port_number) {
    ChargePointConfig cpConfig;

    ESP_LOGI(TAG, "Provisioning charge point info");

    /* #region arg validation */
    if (led_number > 2)
    {
        ESP_LOGE(TAG, "Invalid led number %d (must be 1 or 2)", led_number);
        return 1;
    }

    if (site_name == nullptr || strlen(site_name) == 0)
    {
        ESP_LOGE(TAG, "Invalid site name, cannot be blank");
        return 1;
    }

    if (station_id == nullptr || strlen(station_id) == 0)
    {
        ESP_LOGE(TAG, "Invalid chargepoint station id, cannot be blank");
        return 1;
    }

    if (port_number > 2)
    {
        ESP_LOGE(TAG, "Invalid port number %d (must be 1 or 2)", port_number);
        return 1;
    }

    /* #endregion */

    // Load the config first to make sure we don't overwrite anything we
    // shouldn't.  It is ok if this call fails, probably means the config
    // doesn't exist yet.
    cpConfig.load();

    cpConfig.set_group_id(site_name);
    cpConfig.set_led_station_id_by_led_number(led_number, station_id, port_number);
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
 * information.  The message is sent to the topic site_id/register_station where
 * site_id is the site name where this station is located (or any other grouping)
 * 
 * The message payload is a json string with the following format:
 * {
 *    "station_port": "<station_id>-<port_number>", // 1 index
 *    "thing_led": "<thing_id>-<led_number>",       // 0 index
 * }
 * 
 * The station_port is the charge point station id and the port number separated
 * by a dash.  The thing_led is the thing id and the led number separated by a
 * dash.
 * 
 * The thing id is this device mac address.  It is used when provisioning this
 * device as an AWS iot thing.
 * 
 * @param site_name     Name of the site where this station is located
 * @param led_number    Led number to associate with the charger
 * @param station_id    Charge point station id
 * @param port_number   Charge point station charger port number
 * @return int          0 if successful, 1 otherwise
 */
static int provision_charge_point_info(const char *site_name, uint8_t led_number, const char *station_id, uint8_t port_number) {
    auto &mn8_app = MN8App::instance();

    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *)topic, 64, "%s/register_station", site_name);
    snprintf(
        payload, 256, 
        "{\"station_port\":\"%s-%d\",\"thing_led\":\"%s-%d\"}",
        station_id, port_number, 
        mac_address, led_number-1
    );

    ESP_LOGI(TAG, "Send the register message to topic: %s", topic);
    esp_err_t ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to pair led with charger%s\nMake sure you have a valid network connection", topic);
        return 0;
    }

    memset(topic, 0, sizeof(topic));
    memset(payload, 0, sizeof(payload));
    snprintf((char *)topic, 64, "%s/refresh", site_name);
    snprintf((char *) payload, 256, "{}");

    ESP_LOGI(TAG, "Inform the associated proxy that the data was changed: %s", topic);
    ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Led was paired successfully but proxy may not have refreshed correctly%s\nProxy may need to be manually restarted", topic);
    }
    return ret == ESP_OK ? 0 : 1;
}

//*****************************************************************************
/**
 * @brief Sends the mqtt message to unprovision the charge point information
 * 
 * @param site_name    Name of the site where this station is located
 * @return int 
 */
static int unprovision_charge_point_info(const char *site_name)
{
    char mac_address[13] = {0};
    get_fuse_mac_address_string(mac_address);
    snprintf((char *)topic, 64, "%s/unregister_station", site_name);
    snprintf(payload, 256, "{\"thing\":\"%s\"}", mac_address );

    ESP_LOGI(TAG, "Send the register message to topic: %s", topic);
    auto &mn8_app = MN8App::instance();
    esp_err_t ret = mn8_app.get_mqtt_agent().publish_message(topic, payload, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unpair led with charger%s\nMake sure you have a valid network connection", topic);
        return 0;
    }

    ChargePointConfig cpConfig;
    return cpConfig.reset();
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

    const char *command = chargepoint_command_args.command->sval[0];
    const char *site_name = chargepoint_command_args.site_name->sval[0];
    const uint8_t led_number = chargepoint_command_args.led_number->ival[0];
    const char *station_id = chargepoint_command_args.station_id->sval[0];
    const uint8_t port_number = chargepoint_command_args.port_number->ival[0];

    ESP_LOGI(TAG, "executing command %s", command);
    if (STR_IS_EQUAL(command, "dump")) {
        ESP_LOGI(TAG, "Dumping thing configuration");
        ChargePointConfig cpConfig;
        if (cpConfig.load() == ESP_OK) {
            cpConfig.dump();
        } else {
            ESP_LOGE(TAG, "No charge point info found");
        }
        return 0;
    }

    if (STR_IS_EQUAL(command, "provision")) {
        printf("    led_number: %d\n", led_number);
        printf("    new site_name: %s\n", site_name);
        printf("    new station_id: %s\n", station_id);
        printf("    new port_number: %d\n", port_number);

        // First persists the info to the NVS
        if (persist_charge_point_info(site_name, led_number, station_id, port_number) != 0) {
            ESP_LOGE(TAG, "Failed to persist charge point info");
            return 1;
        }

        // Then send the mqtt message to provision the charge point info.
        // The recipient of the message is the charge point proxy and will
        // take care of updating the database.
        return provision_charge_point_info(site_name, led_number, station_id, port_number);
    }

    if (STR_IS_EQUAL(command, "unprovision")) {
        ESP_LOGI(TAG, "Unprovisioning charge point info");
        return unprovision_charge_point_info(site_name);
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
    chargepoint_command_args.command     = arg_str1(NULL, NULL, "<provision/unprovision>", "provision or unprovision");
    chargepoint_command_args.site_name   = arg_str1(NULL, NULL, "<group or site name>", "Site name where this station is located");
    chargepoint_command_args.led_number  = arg_int1(NULL, NULL, "<1 or 2>", "Led to associate with the charger");
    chargepoint_command_args.station_id  = arg_str1(NULL, NULL, "<station id>", "Charge point station id");
    chargepoint_command_args.port_number = arg_int1(NULL, NULL, "<1 or 2>", "Charger port number");

    chargepoint_command_args.end = arg_end(5);

    const esp_console_cmd_t provision_charge_point_info_cmd = {
        .command = "chargepoint",
        .help = "chargepoint provision/unprovision",
        .hint = NULL,
        .func = &do_chargepoint_command,
        .argtable = &chargepoint_command_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&provision_charge_point_info_cmd));
}
