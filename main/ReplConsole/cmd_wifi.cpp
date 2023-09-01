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

#define STR_IS_EQUAL(str1, str2) (strncasecmp(str1, str2, strlen(str2)) == 0)
#define STR_NOT_EMPTY(str) (strlen(str) > 0)
#define STR_IS_EMPTY(str) (strlen(str) == 0)

static bool wifi_join(const char *ssid, const char *pass, int timeout_ms) {
    MN8App& app = MN8App::instance();
    WifiConnection* wifi_connection = app.get_wifi_connection();
    wifi_creds_t creds = { 0 };
    strlcpy((char *) creds.ssid, ssid, sizeof(creds.ssid));
    if (pass) {
        strlcpy((char *) creds.password, pass, sizeof(creds.password));
    }

    wifi_connection->set_credentials(creds);

    return true;
}

/** Arguments used by 'join' function */
static struct {
    struct arg_int *timeout;
    struct arg_str *command;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

typedef enum {
    e_join_op,
    e_leave_op,
    e_unknown_op
} operation_t;

//*****************************************************************************
static bool is_cmd_valid(const char *cmd)
{
    if (STR_IS_EQUAL(cmd, "join") || 
        STR_IS_EQUAL(cmd, "leave")) {
        return true;
    }
    return false;
}

//*****************************************************************************
static operation_t infer_operation_from_args(void) {
    const char* command = join_args.command->sval[0];
    const char* ssid = join_args.ssid->sval[0];

    if (!is_cmd_valid(command)) {
        printf("ERROR: %s is not a valid command", command);
        return e_unknown_op;
    }

    if (STR_IS_EQUAL(command, "leave")) { return e_leave_op; }

    if (STR_IS_EQUAL(command, "join")) {
        if (STR_IS_EMPTY(ssid)) {
            printf("ERROR: SSID must be specified for config");
            return e_unknown_op;
        }

        return e_join_op;
    }
    return e_unknown_op;
}

//*****************************************************************************
static int on_join(const char *ssid, const char *pass, int timeout_ms) {
    MN8App& app = MN8App::instance();
    WifiConnection* wifi_connection = app.get_wifi_connection();
    wifi_creds_t creds = { 0 };
    strlcpy((char *) creds.ssid, ssid, sizeof(creds.ssid));
    if (pass) {
        strlcpy((char *) creds.password, pass, sizeof(creds.password));
    }

    wifi_connection->set_credentials(creds);

    return 0;
}

//*****************************************************************************
static int on_leave() {
    printf("Leaving wifi\n");
    MN8App& app = MN8App::instance();
    WifiConnection* wifi_connection = app.get_wifi_connection();
    wifi_creds_t creds = { 0 };
    wifi_connection->set_credentials(creds);
    return 0;
}

//*****************************************************************************
static int connect(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }

    /* set default value*/
    if (join_args.timeout->count == 0) {
        join_args.timeout->ival[0] = JOIN_TIMEOUT_MS;
    }

    operation_t op = infer_operation_from_args();
    switch (op) {
        case e_join_op:
            return on_join(join_args.ssid->sval[0], join_args.password->sval[0], join_args.timeout->ival[0]);
        case e_leave_op:
            return on_leave();
        default:
            return 1;
    }

    // Use a timeout here.
    bool connected = wifi_join(join_args.ssid->sval[0],
                               join_args.password->sval[0],
                               join_args.timeout->ival[0]);
    if (!connected) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }

    return 0;
}

void register_wifi(void)
{
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.command = arg_str1(NULL, NULL, "<join/leave>", "Command to execute");
    join_args.ssid = arg_str0(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(3);

    #pragma GCC diagnostic pop "-Wmissing-field-initializers" 
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    const esp_console_cmd_t join_cmd = {
        .command = "wifi",
        .help = "",
        .hint = NULL,
        .func = &connect,
        .argtable = &join_args
    };
    #pragma GCC diagnostic push "-Wmissing-field-initializers" 

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}
