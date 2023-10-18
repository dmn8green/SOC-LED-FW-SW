//*****************************************************************************
/**
 * @file cmd_factory_reset.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief factory repl command implementation
 * @version 0.1
 * @date 2023-10-18
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//*****************************************************************************

#include "cmd_factory_reset.h"

#include "App/Configuration/ThingConfig.h"
#include "App/Configuration/ChargePointConfig.h"
#include "Utils/KeyStore.h"

#include <string.h>
#include "esp_log.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"


static const char *TAG = "factory_reset";

static struct {
    struct arg_end *end;
} factory_reset_args;

//*****************************************************************************
/**
 * @brief register the factory reset command
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
static int do_factory_reset_command(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&factory_reset_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, factory_reset_args.end, argv[0]);
        return 1;
    }

    ESP_LOGI(TAG, "factory reset command");

    KeyStore store;
    if (store.erasePartition() == ESP_OK) {
        printf("factory reset successful\n");
        printf("please reboot the device by typing 'reboot'\n");
        return 0;
    } else {
        printf("factory reset failed\n");
        return 1;
    }
}

//*****************************************************************************
/**
 * @brief register the factory reset command
 */
void register_factory_reset_command(void)
{
    factory_reset_args.end = arg_end(0);

    const esp_console_cmd_t factory_reset_cmd = {
        .command = "factory-reset",
        .help = "factory reset the device",
        .hint = NULL,
        .func = &do_factory_reset_command,
        .argtable = &factory_reset_args
    };
    
    ESP_ERROR_CHECK(esp_console_cmd_register(&factory_reset_cmd));
}
