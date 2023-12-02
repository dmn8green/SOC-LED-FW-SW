//*****************************************************************************
/**
 * @file cmd_iot.cpp
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
//*****************************************************************************

#include "cmd_iot.h"

#include "Utils/iot_provisioning.h"
#include "App/Configuration/ThingConfig.h"

#include <string.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"

static const char *TAG = "provision_iot";
#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)
#define STR_IS_EMPTY(str) (strlen(str) == 0)

static struct {
    struct arg_str *command;
    struct arg_str *username;
    struct arg_str *password;
    struct arg_str *url;
    struct arg_end *end;
} iot_args;

typedef enum {
    e_dump_cmd,
    e_provision_cmd,
    e_unprovision_cmd
} iot_command_t;

//*****************************************************************************
/**
 * @brief Provision/unprovision this device as an AWS iot thing
 * 
 * This function is registered as a REPL command. 
 * It takes the following arguments:
 *  - command: dump/provision/unprovision
 *  - url: url to the provisioning server (optional)
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
static int do_iot_command(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&iot_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, iot_args.end, argv[0]);
        return 1;
    }

    const char * command = iot_args.command->sval[0];
    const char * username = iot_args.username->sval[0];
    const char * password = iot_args.password->sval[0];
    const char * url = iot_args.url->sval[0];

    if (command == nullptr || strlen(command) == 0) {
        command = "dump";
    }

    ESP_LOGI(TAG, "command %s", command);

    if (STR_IS_EQUAL(command, "dump")) {
        ESP_LOGI(TAG, "Dumping thing configuration");
        ThingConfig thingConfig;
        thingConfig.load();
        thingConfig.dump();
        return 0;
    }

    if (STR_IS_EQUAL(command, "unprovision")) {
        ESP_LOGI(TAG, "Unprovisioning device");
        if (unprovision_device(url, username, password)) {
            printf("REBOOTING.....\n");
            esp_restart();
        }
    }

    if (STR_IS_EQUAL(command, "provision")) {
        ESP_LOGI(TAG, "Provisioning device");
        if (provision_device(url, username, password)) {
            printf("REBOOTING.....\n");
            esp_restart();
        }
    }

    ESP_LOGE(TAG, "Invalid command");
    return 1;
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
void register_iot_command(void)
{
    iot_args.command = arg_str0(NULL, NULL, "<command>", "provision or unprovision");
    iot_args.username = arg_str1(NULL, NULL, "<username>", "API username");
    iot_args.password = arg_str1(NULL, NULL, "<password>", "API password");
    iot_args.url = arg_str0(NULL, NULL, "<url>", "Url to provisionign server");
    iot_args.end = arg_end(5);

    const esp_console_cmd_t iot_command = {
        .command = "iot",
        .help = "provision/unprovision device as an iot thing",
        .hint = NULL,
        .func = &do_iot_command,
        .argtable = &iot_args
    };
    
    ESP_ERROR_CHECK(esp_console_cmd_register(&iot_command));
}
