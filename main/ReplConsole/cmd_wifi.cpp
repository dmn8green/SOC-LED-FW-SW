#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "MN8App.h"

#pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
#define JOIN_TIMEOUT_MS (10000)

static bool wifi_join(const char *ssid, const char *pass, int timeout_ms) {
    MN8App& app = MN8App::instance();
    WifiConnection* wifi_connection = app.get_wifi_connection();
    wifi_creds_t creds = { 0 };
    strlcpy((char *) creds.ssid, ssid, sizeof(creds.ssid));
    if (pass) {
        strlcpy((char *) creds.password, pass, sizeof(creds.password));
    }

    wifi_connection->set_credentials(creds);
    wifi_connection->on();
    wifi_connection->connect();
    return true;
}

/** Arguments used by 'join' function */
static struct {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

static int connect(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Connecting to '%s'",
             join_args.ssid->sval[0]);

    // /* set default value*/
    // if (join_args.timeout->count == 0) {
    //     join_args.timeout->ival[0] = JOIN_TIMEOUT_MS;
    // }

    bool connected = wifi_join(join_args.ssid->sval[0],
                               join_args.password->sval[0],
                               join_args.timeout->ival[0]);
    if (!connected) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }

    
    // ESP_LOGI(__func__, "Connected");

    // esp_netif_ip_info_t ip_info;
    // esp_netif_get_ip_info(sta_netif, &ip_info);
    // printf("My IP: " IPSTR "\n", IP2STR(&ip_info.ip));
    // printf("My GW: " IPSTR "\n", IP2STR(&ip_info.gw));
    // printf("My NETMASK: " IPSTR "\n", IP2STR(&ip_info.netmask));

    return 0;
}

void register_wifi(void)
{
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
        .command = "join",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = &connect,
        .argtable = &join_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}
