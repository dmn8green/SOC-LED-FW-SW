/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "udp_server.h"
#include "App/Configuration/ChargePointConfig.h"
#include "Utils/FuseMacAddress.h"
#include "Utils/iot_provisioning.h"
#include "App/Configuration/ThingConfig.h"

#include "rev.h"

#include "ArduinoJson.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define PORT 23269

#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)
#define STR_IS_NULL_OR_EMPTY(str) (str == nullptr || strlen(str) == 0)

//*****************************************************************************
// Static variables
//*****************************************************************************

static const char *TAG = "udp_server";

static char rx_buffer[4096];
static char tx_buffer[8192];

static MN8Context *context = nullptr;

// This is static because it is used in the udp_server_task functions
// We create a json response and the response point to this object
// Because the json response object is created in the main handler
// and the udp_server_task is created in the main handler, we can
// safely assume that this object will be available when the udp_server_task
// is serializing the response.
static ChargePointConfig cpConfig;

//*****************************************************************************
// Forward declarations
//*****************************************************************************
static void udp_server_callback(char *data, int len, char *out, int &out_len);
static void handle_get_chargepoint_config(JsonObject &root, JsonObject &response);
static void handle_set_chargepoint_config(JsonObject &root, JsonObject &response);
static void handle_unprovision_chargepoint(JsonObject &root, JsonObject &response);
static void handle_get_info(JsonObject &root, JsonObject &response);
static void handle_provision_aws(JsonObject &root, JsonObject &response);
static void handle_reboot(JsonObject &root, JsonObject &response);
static void handle_set_led_debug_state(JsonObject &root, JsonObject &response);
static void handle_refresh_proxy(JsonObject &root, JsonObject &response);

// Not sure how to handle this one yet, or if we even need to trouble shoot from
// the device.  We could go through the back door and have a specific rest endpoint
// in aws to ping the device.?
//
// * From the provision UI, user push a get chargepoint latest state button.
// * UI send a udp message to this device.
// * This device will send an mqtt message to the broker to request the latest
//   state of the chargepoint.
// * The broker will poll the chargepoint for its status
// * The broker will send the status to this device via an mqtt message
// * This device will send the status to the provision UI via udp ?
// ^^^^^ Not sure how we can easily do this
//
// Ideas:
// 1) Via udp
// * In the UDP server we keep the address of the client that sent the udp message
// * When we receive the mqtt message from the broker, we send the udp message to
//   the client if the udp server is still running.
// In the client, we use a timeout to bail out if we don't receive the udp message
// from the device in time.
//
// Round trip:
//    --- (1) udp ---  ---   (2) mqtt   ---
//    |             |  |                  |
//    ^             V  ^                  V
//   UI            device               broker --- (3) soap ---> chargepoint
//    ^             V  ^                  V
//    |             |  |                  |
//    --- (1) udp ---  ---   (4) mqtt   ---
static void handle_get_latest_from_proxy(JsonObject &root, JsonObject &response);

//*****************************************************************************
static void udp_server_task(void *pvParameters)
{
    int tx_len = 0;
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    while (1)
    {

        if (addr_family == AF_INET)
        {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(PORT);
            ip_protocol = IPPROTO_IP;
        }

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        // struct timeval timeout;
        // timeout.tv_sec = 10;
        // timeout.tv_usec = 0;
        // setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (1)
        {
            ESP_LOGI(TAG, "Waiting for data");
            memset(rx_buffer, 0, sizeof(rx_buffer));
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                if (source_addr.ss_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                }
                else if (source_addr.ss_family == PF_INET6)
                {
                    inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s: %d", rx_buffer, strlen(rx_buffer));

                if (strlen(rx_buffer) > 1)
                {
                    memset(tx_buffer, 0, sizeof(tx_buffer));
                    tx_len = sizeof(tx_buffer);
                    udp_server_callback(rx_buffer, len, tx_buffer, tx_len);
                }

                // add carriage return
                tx_buffer[strlen(tx_buffer)] = '\n';

                int err = sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

//*****************************************************************************
esp_err_t start_udp_server(MN8Context *context)
{
    ::context = context;
    xTaskCreate(udp_server_task, "udp_server", 16536, (void *)AF_INET, 5, NULL);
    return ESP_OK;
}

//*****************************************************************************
void udp_server_callback(char *data, int len, char *out, int &out_len)
{
    StaticJsonDocument<2048> doc;
    StaticJsonDocument<2048> docResponse;
    char handling_error[512] = {0};
    char mac_address[13] = {0};

    ESP_LOGI(TAG, "udp_server_callback: %s", data);

    get_fuse_mac_address_string(mac_address);

    // Create json response and assume it will be not be successful
    JsonObject response = docResponse.createNestedObject("response");
    response["status"] = "err";
    response["mac_address"] = mac_address;

    // Deser the payload. If it fails, return an error
    DeserializationError error = deserializeJson(doc, data, len);
    if (error)
    {
        ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
        snprintf(handling_error, sizeof(handling_error), "deserializeJson failed %s", error.c_str());
        response["message"] = handling_error;
        goto done;
    }
    else
    {
        // Payload is valid json, check if it contains a command
        JsonObject root = doc.as<JsonObject>();
        if (!root.containsKey("command"))
        {
            ESP_LOGE(TAG, "deserializeJson() failed: no cmd specified");
            snprintf(handling_error, sizeof(handling_error), "deserializeJson failed no cmd");
            response["message"] = handling_error;
            goto done;
        }

        ESP_LOGI(TAG, "cmd: %s", (const char *)root["command"]);

        response["command"] = root["command"];

        // handle the command
        if (STR_IS_EQUAL(root["command"], "get-chargepoint-config"))
        {

            ESP_LOGI(TAG, "get-chargepoint-config requested");
            handle_get_chargepoint_config(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "set-chargepoint-config"))
        {

            ESP_LOGI(TAG, "set-chargepoint-config requested");
            handle_set_chargepoint_config(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "unprovision-chargepoint"))
        {

            ESP_LOGI(TAG, "unprovision-chargepoint requested");
            handle_unprovision_chargepoint(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "provision-aws"))
        {

            ESP_LOGI(TAG, "provision-aws requested");
            handle_provision_aws(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "get-info"))
        {

            ESP_LOGI(TAG, "get-info requested");
            handle_get_info(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "reboot"))
        {

            ESP_LOGI(TAG, "reboot requested");
            handle_reboot(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "set-led-debug-state"))
        {

            ESP_LOGI(TAG, "set-led-debug-state requested");
            handle_set_led_debug_state(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "refresh-proxy"))
        {

            ESP_LOGI(TAG, "refresh-proxy requested");
            handle_refresh_proxy(root, response);
        }
        else if (STR_IS_EQUAL(root["command"], "get-latest-from-proxy"))
        {

            ESP_LOGI(TAG, "get-latest-from-proxy requested");
            handle_get_latest_from_proxy(root, response);
        }
        else
        {

            ESP_LOGI(TAG, "unknown command: %s", (const char *)root["command"]);
            snprintf(handling_error, sizeof(handling_error), "err:unknown command");
            response["message"] = handling_error;
            goto done;
        }

        response["status"] = "ok";
    }

done:
    out_len = serializeJson(docResponse, out, out_len);
    ESP_LOGI(TAG, "udp_server_callback: %s %d", out, out_len);
}

//*****************************************************************************
// Command handling functions
//*****************************************************************************

#define VALIDATE_COND(cond, msg)    \
    if (!(cond))                    \
    {                               \
        response["status"] = "err"; \
        response["message"] = msg;  \
        goto err;                   \
    }

//*****************************************************************************
static void handle_get_chargepoint_config(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "get-chargepoint-config requested");
    uint8_t port_number = 0;
    JsonObject data = response.createNestedObject("data");
    JsonObject led_1 = data.createNestedObject("led_1");
    JsonObject led_2 = data.createNestedObject("led_2");

    if (cpConfig.load() != ESP_OK || !cpConfig.is_configured())
    {
        response["status"] = "ok";
        response["message"] = "Chargepoint not configured";
        data["is_configured"] = false;
    }
    else
    {
        response["status"] = "ok";
        data["is_configured"] = true;
        data["group_id"] = cpConfig.get_group_id();

        led_1["station_id"] = cpConfig.get_led_1_station_id(port_number);
        led_1["port_number"] = port_number;

        led_2["station_id"] = cpConfig.get_led_2_station_id(port_number);
        led_2["port_number"] = port_number;
    }
}

//*****************************************************************************
static void handle_set_chargepoint_config(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "set-chargepoint-config requested");

    JsonObject data = root["data"];

    const char *group_id = data["group_id"];
    const char *led_1_station_id = data["led_1"]["station_id"];
    uint8_t led_1_port_number = data["led_1"]["port_number"];
    const char *led_2_station_id = data["led_2"]["station_id"];
    uint8_t led_2_port_number = data["led_2"]["port_number"];

    ESP_LOGI(TAG, "group_id         : %s", group_id ? group_id : "null");
    ESP_LOGI(TAG, "led_1_station_id : %s", led_1_station_id ? led_1_station_id : "null");
    ESP_LOGI(TAG, "led_2_station_id : %s", led_2_station_id ? led_2_station_id : "null");
    ESP_LOGI(TAG, "led_1_port_number: %d", led_1_port_number);
    ESP_LOGI(TAG, "led_2_port_number: %d", led_2_port_number);

    VALIDATE_COND(!STR_IS_NULL_OR_EMPTY(group_id), "group_id is required");
    VALIDATE_COND(!STR_IS_NULL_OR_EMPTY(led_1_station_id), "led_1_station_id is required");
    VALIDATE_COND(!STR_IS_NULL_OR_EMPTY(led_2_station_id), "led_2_station_id is required");
    VALIDATE_COND(context->get_mqtt_agent().is_connected(), "Not connected to AWS");

    cpConfig.set_chargepoint_info(group_id, led_1_station_id, led_1_port_number, led_2_station_id, led_2_port_number);

    if (context->get_iot_thing().register_cp_station(&cpConfig) != ESP_OK)
    {
        response["message"] = "Failed to broadcast chargepoint config over mqtt";
        goto err;
    }

    if (cpConfig.save() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save chargepoint config");
        response["message"] = "Failed to save chargepoint config";

        // try to rollback
        context->get_iot_thing().unregister_cp_station(&cpConfig);
        goto err;
    }

    response["status"] = "ok";
    response["message"] = "Chargepoint info provisioned";
    return;

err:
    response["status"] = "err";
}

//*****************************************************************************
static void handle_unprovision_chargepoint(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "unprovision-chargepoint requested");

    VALIDATE_COND(context->get_mqtt_agent().is_connected(), "Not connected to AWS");
    VALIDATE_COND(cpConfig.load() == ESP_OK, "Failed to load chargepoint config");
    VALIDATE_COND(cpConfig.is_configured(), "Not configured");
    VALIDATE_COND(context->get_iot_thing().unregister_cp_station(&cpConfig) == ESP_OK, "Failed to unregister chargepoint");
    VALIDATE_COND(cpConfig.reset() == ESP_OK, "Failed to reset chargepoint config");

    response["status"] = "ok";
    response["message"] = "Chargepoint info unprovisioned";
    return;

err:
    response["status"] = "err";
}

//*****************************************************************************
static void handle_get_info(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "get-info requested");

    JsonObject data = response.createNestedObject("data");

    JsonObject firmware = data.createNestedObject("firmware");
    firmware["version"] = VERSION_STRING;

    JsonObject hardware = data.createNestedObject("hardware");
    hardware["mac_address"] = context->get_mac_address();

    JsonObject thing = data.createNestedObject("aws_config");
    if (context->get_thing_config().is_configured())
    {
        thing["is_configured"] = true;
        thing["thing_id"] = context->get_thing_config().get_thing_name();
        thing["endpoint"] = context->get_thing_config().get_endpoint_address();
    }
    else
    {
        thing["is_configured"] = false;
    }

    JsonObject chargepoint = data.createNestedObject("cp_config");
    if (cpConfig.load() == ESP_OK && cpConfig.is_configured())
    {
        uint8_t port_number = 0;

        chargepoint["is_configured"] = true;

        chargepoint["group_id"] = cpConfig.get_group_id();
        JsonObject led_1 = chargepoint.createNestedObject("led_1");
        JsonObject led_2 = chargepoint.createNestedObject("led_2");

        led_1["station_id"] = cpConfig.get_led_1_station_id(port_number);
        led_1["port_number"] = port_number;
        led_1["state"] = context->get_led_task_0().get_state_as_string();

        led_2["station_id"] = cpConfig.get_led_2_station_id(port_number);
        led_2["port_number"] = port_number;
        led_2["state"] = context->get_led_task_1().get_state_as_string();
    }
    else
    {
        chargepoint["is_configured"] = false;
    }

    response["status"] = "ok";
    return;
}

//*****************************************************************************
static void handle_provision_aws(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "provision-aws requested");

    JsonObject data = root["data"];

    if (provision_device("", "admin", "secret"))
    {
        response["status"] = "ok";
        response["message"] = "AWS thing provisioned";
    }
    else
    {
        response["status"] = "err";
        response["message"] = "Failed to provision AWS thing";
    }
}

//*****************************************************************************
static void handle_reboot(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "reboot requested");

    esp_restart();
}

//*****************************************************************************
static void handle_set_led_debug_state(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "set-led-debug-state requested");

    JsonObject data = root["data"];

    uint16_t led = data["led"];
    bool state = data["state"];
    const char *led_state = data["led_state"];
    char final_state[32] = {0};

    if (state && led_state == nullptr)
    {
        snprintf(final_state, sizeof(final_state), "debug_on");
    }
    else
    {
        snprintf(final_state, sizeof(final_state), "debug_off");
    }

    switch (led)
    {
    case 1:
        context->get_led_task_0().set_state(final_state, 0);
        break;
    case 2:
        context->get_led_task_1().set_state(final_state, 0);
        break;
    default:
        response["status"] = "err";
        response["message"] = "Invalid led number";
        return;
    }

    response["status"] = "ok";
    response["message"] = "Led debug state set";
}

//*****************************************************************************
static void handle_refresh_proxy(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "refresh-proxy requested");

    VALIDATE_COND(context->get_mqtt_agent().is_connected(), "Not connected to AWS");
    VALIDATE_COND(cpConfig.load() == ESP_OK, "Failed to load chargepoint config");
    VALIDATE_COND(cpConfig.is_configured(), "Not configured");
    VALIDATE_COND(context->get_iot_thing().force_refresh_proxy(&cpConfig) == ESP_OK, "Failed to refresh proxy");

    response["status"] = "ok";
    response["message"] = "Refreshed proxy";
    return;

err:
    response["status"] = "err";
}

//*****************************************************************************
static void handle_get_latest_from_proxy(JsonObject &root, JsonObject &response)
{
    ESP_LOGI(TAG, "get-latest-from-proxy requested");

    VALIDATE_COND(context->get_mqtt_agent().is_connected(), "Not connected to AWS");
    VALIDATE_COND(cpConfig.load() == ESP_OK, "Failed to load chargepoint config");
    VALIDATE_COND(cpConfig.is_configured(), "Not configured");
    VALIDATE_COND(context->get_iot_thing().request_latest_from_proxy(&cpConfig) == ESP_OK, "Failed to request latest from proxy");

    response["status"] = "ok";
    response["message"] = "Requested latest from proxy";
    return;

err:
    response["status"] = "err";
}
